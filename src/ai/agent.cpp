#include <thread>
#include <iostream>
#include <cstring>
#include <chrono>
#include <fstream>

#include <ai/main.h>

extern "C" {
	#include <rtc.h>
	#include <main.h>
	#include <constants/moves.h>
	#include <platform/dll.h>
	#include <platform/dma.h>
	#include <gba/flash_internal.h>

	// these will cause issues later!
	#undef min
	#undef max
	#undef abs
}

#include <ai/model-manager.h>

#define DATA agentData.data[_storedIndex]
#define POKE(index) DATA.pokemon[index]
#define LOG DATA.log

#define POKE_PARA(poke, prop) eme.GetMonData3(poke, prop, nullptr);

/* helpers for sanity checks */
#define PARTY_RANGE(num)	if(num < 0 || num >= PARTY_SIZE) return;
#define MOVE_RANGE(num)		if(num < 0 || num >= MAX_MON_MOVES) return;

using namespace std::chrono_literals;
#define REG_BASE (eme.REG_BASE)

thread_local static Reward reward = 0;
thread_local static size_t frameNum = 0;
thread_local static volatile uint16_t _storedIndex = 0;
thread_local static struct EmeraldAddresses eme;

/* Reset agent memory so SDL2 doesn't bug out weirdly */
static void resetAgentMemory() {
	for(size_t i = 0;i < PARTY_SIZE;i ++) {
		POKE(i) = AgentPokemonData();
	}
}

enum class RewardType : int64_t {
	MAX =				10,

	/* Pokemon related */
	PokeCaught =		5,
	PokeTraded =		4,
	PokeHatched =		1,
	PokeGotEgg =		2,
	PokeLevelUp =		1,
};

static void gatherState(State& state) {
	#define NEXT(value) state(pos++) = value;
	#define NORMALIZE(input, max) NEXT((input) / (double) (max))

	size_t pos = 0;

	// normalize party pokemon
	for(size_t i = 0;i < PARTY_SIZE;i ++) {
		const auto& poke = POKE(i);
		NORMALIZE(poke.speciesId, NUM_SPECIES)
		NORMALIZE(poke.abilityId, ABILITIES_COUNT)
		NORMALIZE(poke.heldItemId, ITEMS_COUNT)
		NORMALIZE(poke.level, 100)
		NORMALIZE((uint8_t)(poke.typeIds[0] + 1), NUMBER_OF_MON_TYPES + 1)
		NORMALIZE((uint8_t)(poke.typeIds[1] + 1), NUMBER_OF_MON_TYPES + 1)

		for(size_t m = 0;m < MAX_MON_MOVES;m ++) {
			NORMALIZE(poke.moveId[m], MOVES_COUNT)

			if(poke.maxPP[m] == 0) {
				NEXT(0)
			} else {
				NORMALIZE(poke.movePP[m], poke.maxPP[m])
			}
		}
	}

	if (!arma::is_finite(state)) {
		throw std::runtime_error("NaN or Inf in gatherState()!");
	}
}

static inline void ChangeReward(const RewardType type) {
	reward += ((double) type / (double) RewardType::MAX);
}

static void SetGameState(DLL_GameState state) {

}

static const RewardType _Pokemon_Got_RewardType[] = {
	[DLL_Pokemon_Get_Type_CAUGHT] =		RewardType::PokeCaught,
	[DLL_Pokemon_Get_Type_TRADE] =		RewardType::PokeTraded,
	[DLL_Pokemon_Get_Type_HATCHED] =	RewardType::PokeHatched,
	[DLL_Pokemon_Get_Type_EGG] =		RewardType::PokeGotEgg,
};

static const char* _Pokemon_Got_TypeStr[] = {
	[DLL_Pokemon_Get_Type_CAUGHT] =		"caught",
	[DLL_Pokemon_Get_Type_TRADE] =		"received in trade",
	[DLL_Pokemon_Get_Type_HATCHED] =	"hatched",
	[DLL_Pokemon_Get_Type_EGG] =		"received as an egg",
};

static void _Pokemon_Got(DLL_Pokemon_Get_Type type, struct Pokemon* data) {
	if(type < 0 || type >= DLL_Pokemon_Get_Type_EGG) {
		LOG.Error(frameNum, "Pokemon_Got: Type %d is not a valid type!", (int) type);
		return;
	}

	// get Pokemon nickname(can be species or nickname!)
	u8 nickname[POKEMON_NAME_LENGTH + 1];
	eme.GetMonData3(data, MON_DATA_NICKNAME, nickname);
	eme.StringGet_Nickname(nickname);
	const auto _str = EmeraldStringToUTF8(nickname);
	const auto nickString = std::string(_str.cbegin(), _str.cend());

	ChangeReward(_Pokemon_Got_RewardType[type]);
	LOG.Info(frameNum, "Pokemon_Got: Pokemon called %s %s.", nickString.c_str(), _Pokemon_Got_TypeStr[type]);
}

static void _PokemonTeam_Own_Status(int pi, uint32_t status, struct Pokemon* data) {
	PARTY_RANGE(pi)

	LOG.Info(frameNum,
		"PokemonTeam_Own_Status: Pokemon %d called %s status set to %u",
		pi, POKE(pi).nickname.c_str(),
		status
	);
}

static void _PokemonTeam_Own_LevelUp(int pi, struct Pokemon* data) {
	PARTY_RANGE(pi)

	// update level
	POKE(pi).level = POKE_PARA(data, MON_DATA_LEVEL);
	ChangeReward(RewardType::PokeLevelUp);

	LOG.Info(frameNum,
		"PokemonTeam_Own_LevelUp: Pokemon %d called %s leveled up to %u",
		pi, POKE(pi).nickname.c_str(), POKE(pi).level
	);
}

static void _PokemonTeam_Own_UpdatePP(int pi, int mi, struct Pokemon* data) {
	PARTY_RANGE(pi)
	PARTY_RANGE(mi)

	// update PP
	POKE(pi).movePP[mi] = POKE_PARA(data, MON_DATA_PP1 + mi);
}

static void _PokemonTeam_Own_UpdateMove(int pi, int mi, struct Pokemon* data) {
	PARTY_RANGE(pi)
	PARTY_RANGE(mi)

	// get move PP bonus
	uint8_t ppbonus = POKE_PARA(data, MON_DATA_PP_BONUSES);

	// load move parameters
	POKE(pi).moveId[mi] = POKE_PARA(data, MON_DATA_MOVE1 + mi);
	POKE(pi).movePP[mi] = POKE_PARA(data, MON_DATA_PP1 + mi);
	POKE(pi).maxPP[mi] = eme.CalculatePPWithBonus(POKE(pi).moveId[mi], ppbonus, mi);

	// convert move name to string
	auto _str = EmeraldStringToUTF8(eme.gMoveNames[POKE(pi).moveId[mi]]);
	auto moveName = POKE(pi).moveName[mi] = std::string(_str.cbegin(), _str.cend());

	LOG.Info(frameNum,
		"PokemonTeam_Own_UpdateMove: Pokemon %d called %s move %d called %s with PP %u / %u",
		pi, POKE(pi).nickname.c_str(),
		mi, moveName.c_str(),
		POKE(pi).movePP[mi], POKE(pi).maxPP[mi]
	);
}

static void _PokemonTeam_Own_Update(int pi, struct Pokemon* data) {
	PARTY_RANGE(pi)

	if(data == nullptr || !data->box.hasSpecies) {
		POKE(pi).raw = nullptr;
		POKE(pi).nickname = "<null>";

		LOG.Info(frameNum,"PokemonTeam_Own_Update: Pokemon %d is empty", pi);
		return;
	}

	DATA.pokemon[pi].raw = data;

	// load pokemon parameters
	POKE(pi).speciesId = POKE_PARA(data, MON_DATA_SPECIES);
	POKE(pi).heldItemId = POKE_PARA(data, MON_DATA_HELD_ITEM);
	POKE(pi).level = POKE_PARA(data, MON_DATA_LEVEL);

	const auto abilityNum = POKE_PARA(data, MON_DATA_ABILITY_NUM);
	POKE(pi).abilityId = eme.gSpeciesInfo[POKE(pi).speciesId].abilities[abilityNum & 1];

	POKE(pi).typeIds[0] = eme.gSpeciesInfo[POKE(pi).speciesId].types[0];
	POKE(pi).typeIds[1] = eme.gSpeciesInfo[POKE(pi).speciesId].types[1];

	#ifdef ENABLE_SDL2
		// update various strings
		auto _str = EmeraldStringToUTF8(eme.gAbilityNames[POKE(pi).abilityId]);
		POKE(pi).abilityName = std::string(_str.cbegin(), _str.cend());

		if(POKE(pi).heldItemId == ITEM_NONE) {
			POKE(pi).heldItemName = "none";		// GRRRR

		} else {
			_str = EmeraldStringToUTF8(eme.gItems[POKE(pi).heldItemId].name);
			POKE(pi).heldItemName = std::string(_str.cbegin(), _str.cend());
		}
	#endif

	// update Pokemon nickname (can be species or nickname!)
	u8 nickname[POKEMON_NAME_LENGTH + 1];
	eme.GetMonData3(data, MON_DATA_NICKNAME, nickname);
	eme.StringGet_Nickname(nickname);
	_str = EmeraldStringToUTF8(nickname);
	POKE(pi).nickname = std::string(_str.cbegin(), _str.cend());

	// update all moves
	for(uint8_t i = 0; i < MAX_MON_MOVES; i++) {
		_PokemonTeam_Own_UpdateMove(pi, i, data);
	}

	LOG.Info(frameNum,
		"PokemonTeam_Own_Update: Pokemon %d called %s",
		pi, POKE(pi).nickname.c_str()
	);
}

static const struct DLL_Events dll_events = {
	.SetGameState = SetGameState,
	.Pokemon_Got = _Pokemon_Got,
	.PokemonTeam_Own_Update = _PokemonTeam_Own_Update,
	.PokemonTeam_Own_LevelUp = _PokemonTeam_Own_LevelUp,
	.PokemonTeam_Own_Status = _PokemonTeam_Own_Status,
	.PokemonTeam_Own_UpdatePP = _PokemonTeam_Own_UpdatePP,
	.PokemonTeam_Own_UpdateMove = _PokemonTeam_Own_UpdateMove,
};

void VBlankIntrWait() {
	REG_VCOUNT = 161;
	REG_DISPSTAT |= INTR_FLAG_VBLANK;
	eme.RunDMAs(DMA_HBLANK);

	if (REG_DISPSTAT & DISPSTAT_VBLANK_INTR) {
		eme.gIntrTable[4]();
	}

	REG_DISPSTAT &= ~INTR_FLAG_VBLANK;
}

thread_local struct SiiRtcInfo internalClock;

void Platform_GetStatus(struct SiiRtcInfo *rtc){
	rtc->status = internalClock.status;
}

void Platform_SetStatus(struct SiiRtcInfo *rtc){
	internalClock.status = rtc->status;
}

u8 BinToBcd(u8 bin) {
	int placeCounter = 1;
	u8 out = 0;
	do {
		out |= (bin % 10) * placeCounter;
		placeCounter *= 16;

	} while ((bin /= 10) > 0);

	return out;
}

static void UpdateInternalClock(void){
	time_t rawTime = time(NULL);
	struct tm *time = localtime(&rawTime);

	internalClock.year = BinToBcd(time->tm_year - 100);
	internalClock.month = BinToBcd(time->tm_mon + 1);
	internalClock.day = BinToBcd(time->tm_mday);
	internalClock.dayOfWeek = BinToBcd(time->tm_wday);
	internalClock.hour = BinToBcd(time->tm_hour);
	internalClock.minute = BinToBcd(time->tm_min);
	internalClock.second = BinToBcd(time->tm_sec);
}

void Platform_GetDateTime(struct SiiRtcInfo *rtc){
	UpdateInternalClock();

	rtc->year = internalClock.year;
	rtc->month = internalClock.month;
	rtc->day = internalClock.day;
	rtc->dayOfWeek = internalClock.dayOfWeek;
	rtc->hour = internalClock.hour;
	rtc->minute = internalClock.minute;
	rtc->second = internalClock.second;
}

void Platform_SetDateTime(struct SiiRtcInfo *rtc) {
	internalClock.month = rtc->month;
	internalClock.day = rtc->day;
	internalClock.dayOfWeek = rtc->dayOfWeek;
	internalClock.hour = rtc->hour;
	internalClock.minute = rtc->minute;
	internalClock.second = rtc->second;
}

void Platform_GetTime(struct SiiRtcInfo *rtc) {
    UpdateInternalClock();

	rtc->hour = internalClock.hour;
	rtc->minute = internalClock.minute;
	rtc->second = internalClock.second;
}

void Platform_SetTime(struct SiiRtcInfo *rtc) {
	internalClock.hour = rtc->hour;
	internalClock.minute = rtc->minute;
	internalClock.second = rtc->second;
}

void Platform_ReadFlash(u16 sectorNum, u32 offset, u8 *dest, u32 size) {
	std::memcpy(dest, flash + offset + (sectorNum << (*eme.gFlash)->sector.shift), size);
}

void Platform_SetAlarm(u8 *alarmData) {

}

void Platform_StoreSaveFile(void) {

}

void Platform_QueueAudio(float *audioBuffer, s32 samplesPerFrame) {
	std::cerr << "Platform_QueueAudio()" << std::endl;
}

void SoftReset(u32 flags) {
	std::cerr << "SoftReset()" << std::endl;
}

thread_local static uint16_t predictedInput;

u16 Platform_GetKeyInput(void){
	#ifdef ENABLE_SDL2
		// check for overriding agent controls with SDL2 controls
		if(sdlState.gos.userAgentControl == _storedIndex) {
			return sdlState.gos.userInput;
		}
	#endif

	return predictedInput;
}

static const struct DLL_Platform dll_platform = {
	.VBlankIntrWait = VBlankIntrWait,
	.SoftReset = SoftReset,
	.GetKeyInput = Platform_GetKeyInput,
	.StoreSaveFile = Platform_StoreSaveFile,
	.ReadFlash = Platform_ReadFlash,
	.QueueAudio = Platform_QueueAudio,
	.GetStatus = Platform_GetStatus,
	.SetStatus = Platform_SetStatus,
	.GetDateTime = Platform_GetDateTime,
	.SetDateTime = Platform_SetDateTime,
	.GetTime = Platform_GetTime,
	.SetTime = Platform_SetTime,
	.SetAlarm = Platform_SetAlarm,
	.HasAudio = false,
	.SkipToGame = true,
};

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

size_t fpsNotUpdated = 0;
uint8_t missedFramesCount = 0;

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

static uint8_t EpsilonGreedyButtonTable[64] = {
	2, 3, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	4, 5, 6, 7, 4, 5, 6, 7, 4, 5, 6, 7, 4, 5, 6, 7,
	4, 5, 6, 7, 4, 5, 6, 7, 4, 5, 6, 7, 4, 5, 6, 7,
	4, 5, 6, 7, 4, 5, 6, 7, 4, 5, 6, 7, 4, 5, 6, 7,
};

static Action predictInput(uint16_t index, State& state) {
	arma::colvec output(MODEL_ACTION_SIZE);
	agentModelCopies[index].Predict(state, output);

	// Threshold for binary multi-button action
	Action action(output.n_elem);

	for(size_t i = 0; i < output.n_elem; ++i) {
		action(i) = (output(i) > 0.5) ? 1 : 0;
	}

	if (rand() / double(RAND_MAX) < EPSILON_GREEDY_CHANCE) {
		/* Epsilon-greedy: randomly flip input states (directions, A, B, Start, Select) */
		auto index = EpsilonGreedyButtonTable[rand() % 64];
		action(index) = action(index) == 0 ? 1 : 0;
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
		predictedInput |= action(i) << i;
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

	// Load the emerald.dll for this agent
	auto dllHandle = LoadEmeraldDLL(&eme, index);
	if(!dllHandle) {
		return;
	}

	srand(time(NULL) + index + generation);

	/* Initialize the internal clock of this agent. This is used for GBA-compatibility purposes. */
	std::memset(&internalClock, 0, sizeof(internalClock));
	internalClock.status = SIIRTCINFO_24HOUR;
	UpdateInternalClock();

	/* Initialize our own memory */
	resetAgentMemory();
	DATA.active = true;

	/* Set platform functions and initialize the game. */
	eme.Platform_Set(&dll_platform);
	eme.Platform_EventSet(&dll_events);
	REG_VCOUNT = 161;
	eme.AgbInit();

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
		auto action = predictInput(index, startState);
		setInputFromAction(action);

		/* Run for number of frames before AI is polled for inputs */
		for(int i = 0; i < AI_FRAMES_BEFORE_POLL; i++) {
			++frameNum;
			eme.AgbRunFrame();
			VBlankIntrWait();
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
			startState, endState, action, reward
		);

		/* Change to endState to be the new startState */
		startState = endState;
		checkDrawUpdate(index, true);
	}

	exit:
	LOG.Debug(
		frameNum, "Agent %d: Cleaning up simulation. Final reward = %f in generation %zu",
		index, reward, generation
	);

	/* Initialize our own memory */
	DATA.active = false;
	resetAgentMemory();

	// Flush and close log file
	LOG.logStream = nullptr;
	logStream.flush();
	logStream.close();

	// Clean up DLL
	UnloadEmeraldDLL(dllHandle);

	{
		/* Let main thread know this agent is done executing */
		std::lock_guard<std::mutex> lock(agentMutex);
		agentData.agentsCounter = agentData.agentsCounter - 1;
	};
}
