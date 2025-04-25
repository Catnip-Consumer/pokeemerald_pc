#pragma once

#include <filesystem>
#include <mutex>
#include <thread>

extern "C" {
	#include <global.h>
	#include <main.h>
	#include <gba/flash_internal.h>
}

// Save game is loaded to flash buffer and is read-only for agents.
extern uint8_t flash[sizeof(FLASH_BASE)];

/* Determines what the agents should be doing right now */
enum class AgentState {
	WAIT_SYNC,
	RUNNING,
	EXIT,
};

extern volatile AgentState agentState;

/* struct for agent state that may need to be accessed from agent threads */
struct AgentDataStruct {
	// Counts the number of agents that have completed the current task (eg done init, done simulating).
	volatile int32_t agentsCounter;
};

extern AgentDataStruct agentData;
extern std::mutex agentMutex;

#ifdef ENABLE_SDL2
	extern volatile size_t sdlCurrentFrame;

	// Dimensions of the GBA screen in pixels
	#define DISPLAY_WIDTH  240
	#define DISPLAY_HEIGHT 160

	// Define number of agents
	#define GRID_ROWS 8
	#define GRID_COLS 8
	#define CONCURRENT_AGENTS (GRID_ROWS * GRID_COLS)

	extern uint16_t screens[CONCURRENT_AGENTS][DISPLAY_WIDTH * DISPLAY_HEIGHT];
#endif

/* Struct holding various info about the emerald.dll addresses that we use to interface with it. */
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

extern void runAgent(int generation, int index);
extern void DrawFrame(uint16_t *pixels, struct EmeraldAddresses* eme);

/* System-dependent. See win32.cpp, linux.cpp and macos.cpp. */
extern void* GetProcAddress(void* dll, const char* procName);
extern bool GetEmeraldDLLAddresses(struct EmeraldAddresses* eme, void* dll);
extern void* LoadEmeraldDLL(struct EmeraldAddresses* eme, int index);
extern void UnloadEmeraldDLL(void* dll);

extern bool setThreadAffinity(void* handle, bool core0);
extern bool closeThreadHandle(void* handle);
extern void* getCurrentThreadHandle();
extern const std::filesystem::path getExecutableDir();
