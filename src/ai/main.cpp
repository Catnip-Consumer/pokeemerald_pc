#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <thread>

#ifdef ENABLE_SDL2
	#include <SDL2/SDL.h>
#endif

#include <ai/main.h>

volatile uint16_t keys = 0;

int main(int argc, char **argv) {
	/*
	 * Set our affinity to core0. No agent should have permission to run on core0.
	 * This ensures that SDL2 can render and the computer is somewhat usable.
	 */
	auto threadHandle = getCurrentThreadHandle();
	setThreadAffinity(threadHandle, true);
	closeThreadHandle(threadHandle);

	/* Start model thread */
	std::thread model(runModelThread);

#ifdef ENABLE_SDL2
	initSDL();
#endif

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
	agentState = AgentState::EXIT;
	#ifdef ENABLE_SDL2
		exitSDL();
	#endif

	/* Wait for the model thread to be done, before we finally exit. */
	model.join();
	std::cout << "Goodbye." << std::endl;
	return 0;
}
