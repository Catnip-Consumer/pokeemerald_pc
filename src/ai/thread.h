#pragma once

#include <stdint.h>

extern "C" {
	#include <global.h>
	#include <gba/flash_internal.h>
}

extern uint8_t flash[sizeof(FLASH_BASE)];
extern void runAgent(int generation, int index);
extern volatile bool agentStop;

#ifdef ENABLE_SDL2
	extern volatile size_t currentFrame;
	extern volatile int32_t agentsFinishedDrawing;

	// Dimensions of the GBA screen in pixels
	#define DISPLAY_WIDTH  240
	#define DISPLAY_HEIGHT 160

	// Define number of agents
	#define GRID_ROWS 8
	#define GRID_COLS 8
	#define CONCURRENT_AGENTS (GRID_ROWS * GRID_COLS)

	extern uint16_t screens[CONCURRENT_AGENTS][DISPLAY_WIDTH * DISPLAY_HEIGHT];
#endif
