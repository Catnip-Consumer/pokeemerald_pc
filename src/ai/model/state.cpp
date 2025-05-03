#include <mlpack/core.hpp>
#include <ai/model/agent.h>

extern "C" {
	#include <field_control_avatar.h>
	#include <constants/moves.h>
	#include <platform/dll.h>
	#include <platform/dma.h>
	#include <constants/layouts.h>

	// these will cause issues later!
	#undef min
	#undef max
	#undef abs
}


#define ST_CLAMP(value) std::max(0.0, std::min(1.0, value))
#define ST_NEXT(value) state(pos++) = value;
#define ST_NORMALIZE(input, max) ST_NEXT((input) / (double) (max))
#define ST_NORMALIZE_CLAMP(input, max) ST_NEXT(ST_CLAMP((input) / (double) (max)))

static void gatherOverworldState(State& state, size_t& pos) {
	// get overworld position
	struct MapPosition position;
	eme.GetPlayerPosition(&position);

	ST_NORMALIZE_CLAMP(position.x, eme.gBackupMapLayout->width)
	ST_NORMALIZE_CLAMP(position.y, eme.gBackupMapLayout->height)
	ST_NORMALIZE_CLAMP(position.elevation, 16)

	// get map details
	if(eme.gSaveBlock1Ptr == nullptr) {
		ST_NEXT(0.0)

	} else {
		ST_NORMALIZE(eme.gSaveBlock1Ptr->mapLayoutId, LAYOUT_SOOTOPOLIS_CITY_MYSTERY_EVENTS_HOUSE_1F_STAIRS_UNBLOCKED + 1)
	}
}

static void gatherPlayerPartyState(State& state, size_t& pos) {
	// normalize party pokemon
	for(size_t i = 0;i < PARTY_SIZE;i ++) {
		const auto& poke = POKE(i);

		if(poke.raw == nullptr) {
			ST_NEXT(0.0)
			ST_NEXT(0.0)
			ST_NEXT(0.0)
			ST_NEXT(0.0)
			ST_NEXT(0.0)
			ST_NEXT(0.0)
			ST_NEXT(0.0)

		} else {
			ST_NORMALIZE(poke.speciesId, NUM_SPECIES)
			ST_NORMALIZE(poke.abilityId, ABILITIES_COUNT)
			ST_NORMALIZE(poke.heldItemId, ITEMS_COUNT)
			ST_NORMALIZE((uint8_t)(poke.typeIds[0] + 1), NUMBER_OF_MON_TYPES + 1)
			ST_NORMALIZE((uint8_t)(poke.typeIds[1] + 1), NUMBER_OF_MON_TYPES + 1)

			ST_NORMALIZE_CLAMP(poke.raw->hp, poke.raw->maxHP)
			ST_NORMALIZE_CLAMP(poke.level, 100)
		}

		for(size_t m = 0;m < MAX_MON_MOVES;m ++) {
			ST_NORMALIZE(poke.moveId[m], MOVES_COUNT)

			if(poke.maxPP[m] == 0) {
				ST_NEXT(0.0)
			} else {
				ST_NORMALIZE(poke.movePP[m], poke.maxPP[m])
			}
		}
	}
}

void gatherState(State& state) {
	size_t pos = 0;
	gatherOverworldState(state, pos);
	gatherPlayerPartyState(state, pos);

	if (!arma::is_finite(state)) {
		throw std::runtime_error("NaN or Inf in gatherState()!");
	}
}
