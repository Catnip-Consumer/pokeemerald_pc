#pragma once

#include <ai/main.h>
#include <ai/model/manager.h>

extern "C" {
	#include <platform/dll.h>
}

#define LOG DATA.log

#define DATA agentData.data[_storedIndex]
#define POKE(index) DATA.pokemon[index]
#define POKE_PARA(poke, prop) eme.GetMonData3(poke, prop, nullptr);

/* helpers for sanity checks */
#define PARTY_RANGE(num)	if(num < 0 || num >= PARTY_SIZE) return;
#define MOVE_RANGE(num)		if(num < 0 || num >= MAX_MON_MOVES) return;

extern thread_local struct EmeraldAddresses eme;
extern thread_local volatile uint16_t _storedIndex;
extern thread_local uint16_t predictedInput;

extern thread_local Reward lastReward;
extern thread_local Reward reward;
extern thread_local size_t frameNum;
extern thread_local size_t flashId;

extern const struct DLL_Events dll_events;

enum class RewardType : int64_t {
	MAX =				100,

	/* Pokemon related */
	PokeCaught =		50,
	PokeTraded =		40,
	PokeHatched =		10,
	PokeGotEgg =		20,
	PokeLevelUp =		15,
};

inline void ChangeReward(const RewardType type) {
	reward += ((double) type / (double) RewardType::MAX);
}

extern void gatherState(State& state);
extern void initEmerald(uint16_t index);
extern void unloadEmerald();
extern void emeraldFrame();
