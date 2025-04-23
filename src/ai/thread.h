#pragma once

#include <filesystem>
#include <mutex>

extern "C" {
	#include <global.h>
	#include <main.h>
	#include <gba/flash_internal.h>
}

extern uint8_t flash[sizeof(FLASH_BASE)];
extern void runAgent(int generation, int index);
extern volatile bool agentStop;
extern volatile bool agentWaitSync;

extern std::mutex agentMutex;
extern volatile size_t agentWaitingSync;

#ifdef ENABLE_SDL2
	extern std::mutex sdlMutex;
	extern volatile size_t sdlCurrentFrame;
	extern volatile int32_t sdlAgentsFinishedDrawing;

	// Dimensions of the GBA screen in pixels
	#define DISPLAY_WIDTH  240
	#define DISPLAY_HEIGHT 160

	// Define number of agents
	#define GRID_ROWS 8
	#define GRID_COLS 8
	#define CONCURRENT_AGENTS (GRID_ROWS * GRID_COLS)

	extern uint16_t screens[CONCURRENT_AGENTS][DISPLAY_WIDTH * DISPLAY_HEIGHT];
#endif

const std::filesystem::path getExecutableDir();

struct EmeraldAddresses {
	void (*Platform_Set)(const struct DLL_Platform*);
	void (*RunDMAs)(u32);
	void (*AgbInit)();
	void (*AgbRunFrame)();

	IntrFunc* gIntrTable;
	const struct FlashType** gFlash;
	unsigned char* REG_BASE;

	#ifdef ENABLE_SDL2
		unsigned char* VRAM_;
		unsigned char* PLTT;
		unsigned char* OAM;
	#endif
};
