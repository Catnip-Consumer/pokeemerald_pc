#ifndef GUARD_DLL_H
#define GUARD_DLL_H

#include "global.h"

typedef enum DLL_GameState {
	DLL_GameState_IGNORE,					// Miscellaneous state, AI should ignore this
	DLL_GameState_OVERWORLD,				// Player is in a map, eg in a town or cave
	DLL_GameState_OVERWORLD_BAG,			// Player is in the bag in the overworld
	DLL_GameState_OVERWORLD_POKEMON,		// Player is in the Pokemon menu in the overworld
	DLL_GameState_OVERWORLD_POKEMON_SUMMARY,// Player is viewing a Pokemon summary in the overworld

	DLL_GameState_BATTLE,					// Player is in a battle
	DLL_GameState_BATTLE_BAG,				// Player is in the bag during a battle
	DLL_GameState_BATTLE_POKEMON,			// Player is selecting a Pokemon during a battle
	DLL_GameState_BATTLE_MOVE,				// Player is selecting a move during a battle
} DLL_GameState;

struct DLL_Events {
	/* General game state */
	void (*SetGameState)(DLL_GameState state);

	/* Pokemon team related */
	void (*PokemonTeam_Own_Update)(int index, struct Pokemon* data);
};

struct DLL_Platform {
	void (*VBlankIntrWait)(void);
	void (*SoftReset)(u32);
	u16 (*GetKeyInput)(void);
	void (*StoreSaveFile)(void);
	void (*ReadFlash)(u16 sectorNum, u32 offset, u8 *dest, u32 size);
	void (*QueueAudio)(float *audioBuffer, s32 samplesPerFrame);
	void (*GetStatus)(struct SiiRtcInfo *rtc);
	void (*SetStatus)(struct SiiRtcInfo *rtc);
	void (*GetDateTime)(struct SiiRtcInfo *rtc);
	void (*SetDateTime)(struct SiiRtcInfo *rtc);
	void (*GetTime)(struct SiiRtcInfo *rtc);
	void (*SetTime)(struct SiiRtcInfo *rtc);
	void (*SetAlarm)(u8 *alarmData);
	bool8 HasAudio;
	bool8 SkipToGame;
};

_DLL_ void Platform_Set(const struct DLL_Platform *platform);
_DLL_ void Platform_EventSet(const struct DLL_Events *events);

/* THE FOLLOWINB ARE ONLY TO BE USED WITH THE GAME. DLL SHOULD IGNORE THESE */
extern const struct DLL_Events *gDllEvents;

#define EV_FORWARD(func, ...) \
	if(gDllEvents) gDllEvents->func(__VA_ARGS__);

inline void PokemonTeam_Own_Update(int index, struct Pokemon* data) {
	EV_FORWARD(PokemonTeam_Own_Update, index, data);
}

#endif
