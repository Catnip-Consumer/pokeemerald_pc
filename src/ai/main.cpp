#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <thread>

#ifdef ENABLE_SDL2
	#include <SDL2/SDL.h>
#endif

#include <ai/main.h>

uint8_t flash[sizeof(FLASH_BASE)];
volatile uint16_t keys = 0;

std::mutex agentMutex;
AgentDataStruct agentData;
volatile AgentState agentState;

static void ReadSaveFile() {
	// fill flash buffer with 0xFF and read contents
	memset(flash, 0xFF, sizeof(flash));
	std::ifstream savefile;

	try {
		const auto savePath = getExecutableDir() / "emerald-ai.sav";
		savefile = std::ifstream(savePath, std::ios::binary);

		// get file size
		savefile.seekg(0, std::ios::end);
		const auto size = savefile.tellg();
		savefile.seekg(0, std::ios::beg);

		// read from file
		const auto readSize = min((std::streampos) size, (std::streampos) sizeof(flash));
		savefile.read(reinterpret_cast<char*>(flash), size);

		std::cout << "Read " << size << " bytes from " << savePath << std::endl;

	} catch (std::exception*) {
		// assume the file was not found
	}

	// close the file
	if (savefile.is_open()) {
		savefile.close();
	}
}

void createDllCopy(size_t num) {
#ifdef _WIN32
	/* Delete the old DLL (should be able to without deletion but I can't be bothered rn) */
	const auto target = getExecutableDir() / "_dll" / ("libemerald_" + std::to_string(num) + ".dll");

	if(std::filesystem::exists(target)) {
		std::filesystem::remove(target);
	}

	std::filesystem::copy_file(getExecutableDir() / "libemerald.dll", target);
#endif
}

int main(int argc, char **argv) {
	/*
	 * Set our affinity to core0. No agent should have permission to run on core0.
	 * This ensures that SDL2 can render and the computer is somewhat usable.
	 */
	auto threadHandle = getCurrentThreadHandle();
	setThreadAffinity(threadHandle, true);
	closeThreadHandle(threadHandle);

	ReadSaveFile();

	/*
	 * Create a directory for DLL's to be dumped on. This method makes sure that Windows thinks we're
	 * opening many DLL's, which ensures thread safety. This is dumb but an easy solution to a hard problem.
	 */
	#ifdef _WIN32
		std::filesystem::create_directory(getExecutableDir() / "_dll");
	#endif

	agentState = AgentState::WAIT_SYNC;
	agentData.agentsCounter = CONCURRENT_AGENTS;
	std::thread agent[CONCURRENT_AGENTS];

	for(int i = 0; i < CONCURRENT_AGENTS; i++) {
		createDllCopy(i);

		// Start the agent thread
		agent[i] = std::thread(runAgent, 0, i);
	}

#ifdef ENABLE_SDL2
	initSDL();
#endif

	/* Wait until each agent has initialized and are waiting for sync */
	std::cout << "Waiting for agents to sync..." << std::endl;

	while(true) {
		#ifdef ENABLE_SDL2
			if(updateSDL()) {
				goto exit_simulation;
			}
		#endif

		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		{
			std::lock_guard<std::mutex> lock(agentMutex);
			if(agentData.agentsCounter <= 0) {
				break;
			}
		}
	}

	{
		/* Agents are now synced, allow them to run their code */
		std::lock_guard<std::mutex> lock(agentMutex);
		agentState = AgentState::RUNNING;
		agentData.agentsCounter = CONCURRENT_AGENTS;
	}

	std::cout << "Running " << CONCURRENT_AGENTS << " agents..." << std::endl;

	while(true) {
		/* Agents are running their code. This thread really only handles SDL2 events (if enabled) */
		#ifdef ENABLE_SDL2
			if(updateSDL()) {
				goto exit_simulation;
			}
		#endif
	}

	/* Agents are now quitting */
	exit_simulation:
	std::cout << "Simulation ending... Do cleanup." << std::endl;

	agentState = AgentState::EXIT;
	#ifdef ENABLE_SDL2
		exitSDL();
	#endif

	for(int i = 0; i < CONCURRENT_AGENTS; i++) {
		agent[i].join();
	}

	std::cout << "Goodbye." << std::endl;
	return 0;
}
