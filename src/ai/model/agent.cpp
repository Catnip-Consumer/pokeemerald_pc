#include <thread>
#include <iostream>
#include <cstring>
#include <chrono>
#include <fstream>

#include <ai/main.h>

extern "C" {
	#include <main.h>
	#include <platform/dll.h>
	#include <platform/dma.h>
	#include <gba/flash_internal.h>

	// these will cause issues later!
	#undef min
	#undef max
	#undef abs
}

#include <ai/model/manager.h>
#include <ai/model/agent.h>

using namespace std::chrono_literals;

thread_local Reward lastReward = 0;
thread_local Reward reward = 0;
thread_local size_t frameNum = 0;
thread_local size_t flashId = 0;
thread_local volatile uint16_t _storedIndex = 0;

thread_local static std::mt19937 mt19937 { std::random_device()() };
thread_local static std::uniform_real_distribution<double> randomFloat(0, 1);
thread_local static std::uniform_real_distribution<double> randomDirection(4, 8);
thread_local static std::uniform_real_distribution<double> randomFramecount(16, 64);

/* Reset agent memory so SDL2 doesn't bug out weirdly */
static void resetAgentMemory() {
	for(size_t i = 0;i < PARTY_SIZE;i ++) {
		POKE(i) = AgentPokemonData();
	}
}

#ifdef ENABLE_SDL2
thread_local static uint8_t lastFrame = -1;

/* Update the number of frames ran to SDL */
void updateFrameCount(uint16_t index, size_t count) {
	auto* fpsAddr = &(sdlState.sds.frameCounts[index][sdlState.gos.fpsIndex]);
	*fpsAddr = count + *fpsAddr;
}

/* aiframe should match this table when reading SDLPlaybackSpeed to check whether to update draw at all. */
constexpr bool isAiFrameUpdate[] = {
	[(size_t) SDLPlaybackSpeed::PAUSED] = false,
	[(size_t) SDLPlaybackSpeed::REALTIME] = false,
	[(size_t) SDLPlaybackSpeed::FAST] = false,
	[(size_t) SDLPlaybackSpeed::SLIDESHOW] = true,
	[(size_t) SDLPlaybackSpeed::MAX] = true,
};

static thread_local size_t fpsNotUpdated = 0;
static thread_local uint8_t missedFramesCount = 0;

bool checkDrawUpdate(uint16_t index, bool aiframe) {
	// Check if the current frame is ai update frame or any frame
	if(aiframe != isAiFrameUpdate[(size_t) sdlState.gos.playbackSpeed]) {
		return false;
	}

	if(!aiframe && missedFramesCount > 0) {
		if(sdlState.gos.currentFrame == lastFrame) {
			// We missed rendering a previous frame
			--missedFramesCount;
			fpsNotUpdated++;
			return false;
		}

		// SDL has requested yet another frame..... Just go render it
		missedFramesCount += sdlState.gos.currentFrame - lastFrame - 1;
		goto renderIt;
	}

	// Check if the current frame is the same as the last frame drawn
	if(sdlState.gos.currentFrame == lastFrame) {
		if(aiframe) {
			// if there isn't a frame available, but this is an ai frame, keep running
			fpsNotUpdated += AI_FRAMES_BEFORE_POLL;
			return false;
		}

		{
			// sleep until frame is received
			std::unique_lock<std::mutex> lock(sdlState.signal.mutex);
			sdlState.signal.cv.wait(lock, [] { return sdlState.gos.currentFrame != lastFrame; });

			// Calculate the number of frames we missed processing
			missedFramesCount = sdlState.gos.currentFrame - lastFrame - 1;
		}
	}

	renderIt:
	// We are here, so that means a new frame was available
	lastFrame = sdlState.gos.currentFrame;

	// update frame index
	updateFrameCount(index, fpsNotUpdated + 1);
	fpsNotUpdated = 0;

	// Render frame to screen buffer
	DrawFrame(sdlState.aos.screens[index], &eme);
	return true;
}

#else
bool checkDrawUpdate(int index, bool aiframe) {
	return false;
}
#endif

static thread_local size_t holdUntilFrame = 0;
static thread_local uint8_t holdDirection = 0;

static Action predictInput(size_t generation, uint16_t index, State& state) {
	// Threshold for binary multi-button action
	Action action(MODEL_ACTION_SIZE);

	if (randomFloat(mt19937) < GetEpsilonGreedyChance(generation)) {
		/* Check if we have chosen to hold some button for a number of frames */
		if(holdUntilFrame <= frameNum) {
			// Choose new direction
			const size_t count = randomFramecount(mt19937);
			holdUntilFrame = frameNum + count;
			holdDirection = randomDirection(mt19937);
		}

		action.zeros();
		action(holdDirection) = 1;

		/* Check if we would like to select a random button to press too */
		const auto randomValue = randomFloat(mt19937);

		if(randomValue < 0.00001) {
			action(2) = 1;		// Select

		} else if(randomValue < 0.00005) {
			action(3) = 1;		// Start

		} else if(randomValue < 0.001) {
			action(1) = 1;		// B

		} else if(randomValue < 0.01) {
			action(0) = 1;		// A
		}

	} else {
		/* Predict the output of the model */
		arma::colvec output(MODEL_ACTION_SIZE);
		agentModelCopies[index].Predict(state, output);

		/* Normalize to either on or off */
		for(size_t i = 0; i < output.n_elem; ++i) {
			action(i) = (output(i) > 0.5) ? 1 : 0;
		}

		/* TEMP: ban start and select from action */
		action(2) = action(3) = 0.0;
	}

	// Check action is valid
	assert(action.n_elem == MODEL_ACTION_SIZE);
	assert(arma::all(action <= 1)); // All values must be 0 or 1
	return action;
}

static void setInputFromAction(Action& action) {
	predictedInput = 0;

	/* Just loop through each action state, and treat it as a bit (on or off) */
	for(size_t i = 0; i < action.n_elem; ++i) {
		predictedInput |= uint16_t(action(i)) << i;
	}
}

void runAgent(size_t generation, uint16_t index) {
	arma::arma_rng::set_seed_random();
	_storedIndex = index;

	/* Log file target for this AI */
	const auto aiDir = getExecutableDir() / ".ai";
	const auto genDir = aiDir / "runs" / ("gen"+ std::to_string(generation));
	const auto logfile = genDir / std::to_string(index) / "log.txt";

	/* Create the directory if it doesn't exist */
	std::filesystem::create_directories(logfile.parent_path());

	/* Open the log file */
	std::ofstream logStream(logfile);
	LOG.logStream = &logStream;

	/* Set affinity to not run on core0. See main.cpp for more info. */
	auto threadHandle = getCurrentThreadHandle();
	setThreadAffinity(threadHandle, false);
	closeThreadHandle(threadHandle);

	/* Initialize the DLL */
	initEmerald(index);

	/* Initialize our own memory */
	srand(time(NULL) + index + generation);
	resetAgentMemory();
	DATA.active = true;

	{
		/* Let main thread know this agent is done initializing */
		std::lock_guard<std::mutex> lock(agentMutex);
		agentData.agentsCounter = agentData.agentsCounter - 1;
	}

	/* Sleep while other agents are still trying to initialize */
	while(AgentState::WAIT_SYNC == agentState) {
		std::this_thread::sleep_for(1ms);
	}

	LOG.Debug(frameNum, "Agent %d: Starting simulation", index);

	/* Set initial state. Any future updates, we just reuse the previous end state */
	State startState(MODEL_STATE_SIZE);
	gatherState(startState);

	while(AgentState::RUNNING == agentState && frameNum < SIMULATION_FRAMECOUNT) {
		/* Predict input based on current state */
		auto action = predictInput(generation, index, startState);
		setInputFromAction(action);

		/* Run for number of frames before AI is polled for inputs */
		for(int i = 0; i < AI_FRAMES_BEFORE_POLL; i++) {
			++frameNum;
			emeraldFrame();
			checkDrawUpdate(index, false);

			if(AgentState::RUNNING != agentState) {
				goto exit;
			}
		}

		/* Gather new data about what just happened */
		State endState(MODEL_STATE_SIZE);
		gatherState(endState);

		/* Store experience */
		replayBuffers[index].emplace_back(
			startState, endState, action, reward, reward - lastReward
		);

		/* Change to endState to be the new startState */
		startState = endState;
		lastReward = reward;
		checkDrawUpdate(index, true);
	}

	exit:
	LOG.Debug(frameNum,
		"Agent %d: Cleaning up simulation. Final reward = %f in generation %zu.",
		index, reward, generation
	);

	/* Initialize our own memory */
	DATA.active = false;
	resetAgentMemory();

	// Flush and close log file
	LOG.logStream = nullptr;
	logStream.flush();
	logStream.close();

	unloadEmerald();

	{
		/* Let main thread know this agent is done executing */
		std::lock_guard<std::mutex> lock(agentMutex);
		agentData.agentsCounter = agentData.agentsCounter - 1;
	};
}
