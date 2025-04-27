#pragma once

#include <filesystem>
#include <mutex>
#include <thread>
#include <condition_variable>

extern "C" {
	#include <global.h>
	#include <main.h>
	#include <gba/flash_internal.h>
}

// Save game is loaded to flash buffer and is read-only for agents.
extern uint8_t flash[sizeof(FLASH_BASE)];

/* Determines what the agents should be doing right now */
enum class AgentState : uint8_t {
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

/* Agent specific variables */
#define AI_TILEMAP_W	(DISPLAY_WIDTH / 16)
#define AI_TILEMAP_H	(DISPLAY_HEIGHT / 16)
#define AI_TILEMAP_SIZE	(AI_TILEMAP_W * AI_TILEMAP_H)

/* System-dependent. See win32.cpp, linux.cpp and macos.cpp. */
extern void* GetProcAddress(void* dll, const char* procName);
extern bool GetEmeraldDLLAddresses(struct EmeraldAddresses* eme, void* dll);
extern void* LoadEmeraldDLL(struct EmeraldAddresses* eme, int index);
extern void UnloadEmeraldDLL(void* dll);

extern bool setThreadAffinity(void* handle, bool core0);
extern bool closeThreadHandle(void* handle);
extern void* getCurrentThreadHandle();
extern const std::filesystem::path getExecutableDir();

// TODO: Obsolete flag, use emerald.dll itself to determine when AI decisions are needed(!)
#define AI_FRAMES_BEFORE_POLL 8

// Define number of agents to run. GRID_ROWS and GRID_COLS are only relevant for SDL2 but also are used to get agent count.
#define GRID_ROWS 8
#define GRID_COLS 8
#define CONCURRENT_AGENTS (GRID_ROWS * GRID_COLS)

#ifdef ENABLE_SDL2
	/* SDL2 lifecycle functions */
	bool initSDL();
	void exitSDL();
	bool updateSDL();
	#define FPS_COUNTS 8

	// SDL playback speed state
	enum class SDLPlaybackSpeed : uint8_t {
		PAUSED,			// 0fps - display every frame, advance only when button pressed
		REALTIME,		// 60fps - display every frame
		FAST,			// 360fps - display every frame
		SLIDESHOW,		// 20fps - display when requested
		MAX,			// 4spf - display when requested
	};

	struct SDLState {
		/* State the agents can only read */
		struct SDLGuiOnlyState {
			volatile uint16_t userInput = 0;
			volatile uint8_t currentFrame = 0;
			volatile int16_t viewIndex = -1;
			volatile uint8_t fpsIndex = 0;
			volatile SDLPlaybackSpeed playbackSpeed = SDLPlaybackSpeed::FAST;
		} gos;

		/* State only the agents can write */
		struct SDLAgentOnlyState {
			uint16_t screens[CONCURRENT_AGENTS][DISPLAY_WIDTH * DISPLAY_HEIGHT];
			uint32_t aiTileMap[AI_TILEMAP_SIZE];
		} aos;

		/* State that needs a mutex to access */
		struct SDLSharedState {

			volatile size_t frameCounts[CONCURRENT_AGENTS][FPS_COUNTS] = {0};
		} sds;

		/* Mutexes for the shared state */
		std::mutex mutexSDS;

		/* Variables for handling waiting on SDL to request the next frame */
		struct {
			std::mutex mutex;
			std::condition_variable cv;
		} signal;
	};

	extern SDLState sdlState;
#endif
