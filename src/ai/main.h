#pragma once

#include <filesystem>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <ai/library/log.h>

extern "C" {
	#include <global.h>
	#include <main.h>
	#include <gba/flash_internal.h>
	#include <item.h>
	#include <constants/abilities.h>
}

// TODO: Obsolete flag, use emerald.dll itself to determine when AI decisions are needed(!)
#define AI_FRAMES_BEFORE_POLL 8

// Define number of agents to run. GRID_ROWS and GRID_COLS are only relevant for SDL2 but also are used to get agent count.
#define GRID_ROWS 3
#define GRID_COLS 3
#define CONCURRENT_AGENTS (GRID_ROWS * GRID_COLS)

// Describes the current generation of the simulation.
extern volatile size_t generation;

/* Determines what the agents should be doing right now */
enum class AgentState : uint8_t {
	WAIT_SYNC,
	RUNNING,
	FINISHED,
	EXIT,
};

extern volatile AgentState agentState;

struct AgentPokemonData {
	struct Pokemon* raw;

	// Move statistics
	uint16_t moveId[MAX_MON_MOVES];
	uint8_t movePP[MAX_MON_MOVES];
	uint8_t maxPP[MAX_MON_MOVES];

	// Pokemon statistics
	uint16_t speciesId;
	uint16_t abilityId;
	uint16_t heldItemId;
	uint8_t typeIds[2];
	uint8_t level;

	std::string nickname;

	#ifdef ENABLE_SDL2
		std::string abilityName;
		std::string heldItemName;
		std::string moveName[MAX_MON_MOVES];
	#endif
};

struct SingleAgentData {
	/* Pokemon data */
	struct AgentPokemonData pokemon[PARTY_SIZE];

	// Agent logger
	Logger log;

	// If false, agent is not active and using any of it's data is invalid
	bool active = false;
};

/* struct for agent state that may need to be accessed from agent threads */
struct AgentDataStruct {
	// Counts the number of agents that have completed the current task (eg done init, done simulating).
	volatile int32_t agentsCounter;

	// Agent data struct for each agent
	struct SingleAgentData data[CONCURRENT_AGENTS];
};

extern AgentDataStruct agentData;
extern std::mutex agentMutex;

/* Struct holding various info about the emerald.dll addresses that we use to interface with it. */
struct EmeraldAddresses {
	void (*Platform_Set)(const struct DLL_Platform*);
	void (*Platform_EventSet)(const struct DLL_Events*);
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

	void (*StringGet_Nickname)(u8*);
	u32 (*GetMonData3)(struct Pokemon *mon, s32 field, u8 *data);
	u8 (*CalculatePPWithBonus)(u16 move, u8 ppBonuses, u8 moveIndex);
	void (*GetPlayerPosition)(struct MapPosition*);

	struct BackupMapLayout* gBackupMapLayout;
	struct SaveBlock1* gSaveBlock1Ptr;
	const struct SpeciesInfo* gSpeciesInfo;
	const u8 (*gMoveNames)[MOVE_NAME_LENGTH + 1];

	#ifdef ENABLE_SDL2
		const u8 (*gAbilityNames)[ABILITY_NAME_LENGTH + 1];
		const struct Item* gItems;
	#endif
};

extern void runModelThread();
extern void runAgent(size_t generation, uint16_t index);
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

#ifdef ENABLE_SDL2
	/* SDL2 lifecycle functions */
	bool initSDL();
	void exitSDL();
	bool updateSDL();
	#define FPS_COUNTS 8

	// SDL playback speed state
	enum class SDLPlaybackSpeed : uint8_t {
		PAUSED,			// 30fps - display every frame, advance only when button pressed
		REALTIME,		// 60fps - display every frame
		FAST,			// 360fps - display every frame
		SLIDESHOW,		// 30fps - display when requested
		MAX,			// 1fps - display when requested
	};

	struct SDLState {
		/* State the agents can only read */
		struct SDLGuiOnlyState {
			volatile size_t userAgentControl = -1;
			volatile uint16_t userInput = 0;
			volatile uint8_t currentFrame = 0;
			volatile uint8_t fpsIndex = 0;
			volatile SDLPlaybackSpeed playbackSpeed = SDLPlaybackSpeed::FAST;
		} gos;

		/* State only the agents can write */
		struct SDLAgentOnlyState {
			uint16_t screens[CONCURRENT_AGENTS][DISPLAY_WIDTH * DISPLAY_HEIGHT];
			uint32_t tilemap[CONCURRENT_AGENTS][AI_TILEMAP_SIZE];
		} aos;

		/* State that needs a mutex to access */
		struct SDLSharedState {
			std::atomic<size_t> frameCounts[CONCURRENT_AGENTS][FPS_COUNTS] = {0};
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

/* Emerald libary functions */
extern std::u8string EmeraldStringToUTF8(const uint8_t* str);
