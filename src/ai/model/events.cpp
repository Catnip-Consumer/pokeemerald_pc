#include <ai/model/manager.h>
#include <ai/model/agent.h>

extern "C" {
	#include <platform/dll.h>

	// these will cause issues later!
	#undef min
	#undef max
	#undef abs
}

static void SetGameState(DLL_GameState state) {

}

static const RewardType _Pokemon_Got_RewardType[] = {
	[DLL_Pokemon_Get_Type_CAUGHT] =		RewardType::PokeCaught,
	[DLL_Pokemon_Get_Type_TRADE] =		RewardType::PokeTraded,
	[DLL_Pokemon_Get_Type_HATCHED] =	RewardType::PokeHatched,
	[DLL_Pokemon_Get_Type_EGG] =		RewardType::PokeGotEgg,
};

static const char* _Pokemon_Got_TypeStr[] = {
	[DLL_Pokemon_Get_Type_CAUGHT] =		"caught",
	[DLL_Pokemon_Get_Type_TRADE] =		"received in trade",
	[DLL_Pokemon_Get_Type_HATCHED] =	"hatched",
	[DLL_Pokemon_Get_Type_EGG] =		"received as an egg",
};

static void _Pokemon_Got(DLL_Pokemon_Get_Type type, struct Pokemon* data) {
	if(type < 0 || type >= DLL_Pokemon_Get_Type_EGG) {
		LOG.Error(frameNum, "Pokemon_Got: Type %d is not a valid type!", (int) type);
		return;
	}

	// get Pokemon nickname(can be species or nickname!)
	u8 nickname[POKEMON_NAME_LENGTH + 1];
	eme.GetMonData3(data, MON_DATA_NICKNAME, nickname);
	eme.StringGet_Nickname(nickname);
	const auto _str = EmeraldStringToUTF8(nickname);
	const auto nickString = std::string(_str.cbegin(), _str.cend());

	ChangeReward(_Pokemon_Got_RewardType[type]);
	LOG.Info(frameNum, "Pokemon_Got: Pokemon called %s %s.", nickString.c_str(), _Pokemon_Got_TypeStr[type]);
}

static void _PokemonTeam_Own_Status(int pi, uint32_t status, struct Pokemon* data) {
	PARTY_RANGE(pi)

	LOG.Info(frameNum,
		"PokemonTeam_Own_Status: Pokemon %d called %s status set to %u",
		pi, POKE(pi).nickname.c_str(),
		status
	);
}

static void _PokemonTeam_Own_LevelUp(int pi, struct Pokemon* data) {
	PARTY_RANGE(pi)

	// update level
	POKE(pi).level = POKE_PARA(data, MON_DATA_LEVEL);
	ChangeReward(RewardType::PokeLevelUp);

	LOG.Info(frameNum,
		"PokemonTeam_Own_LevelUp: Pokemon %d called %s leveled up to %u",
		pi, POKE(pi).nickname.c_str(), POKE(pi).level
	);
}

static void _PokemonTeam_Own_UpdatePP(int pi, int mi, struct Pokemon* data) {
	PARTY_RANGE(pi)
	PARTY_RANGE(mi)

	// update PP
	POKE(pi).movePP[mi] = POKE_PARA(data, MON_DATA_PP1 + mi);
}

static void _PokemonTeam_Own_UpdateMove(int pi, int mi, struct Pokemon* data) {
	PARTY_RANGE(pi)
	PARTY_RANGE(mi)

	// get move PP bonus
	uint8_t ppbonus = POKE_PARA(data, MON_DATA_PP_BONUSES);

	// load move parameters
	POKE(pi).moveId[mi] = POKE_PARA(data, MON_DATA_MOVE1 + mi);
	POKE(pi).movePP[mi] = POKE_PARA(data, MON_DATA_PP1 + mi);
	POKE(pi).maxPP[mi] = eme.CalculatePPWithBonus(POKE(pi).moveId[mi], ppbonus, mi);

	// convert move name to string
	auto _str = EmeraldStringToUTF8(eme.gMoveNames[POKE(pi).moveId[mi]]);
	auto moveName = POKE(pi).moveName[mi] = std::string(_str.cbegin(), _str.cend());

	LOG.Info(frameNum,
		"PokemonTeam_Own_UpdateMove: Pokemon %d called %s move %d called %s with PP %u / %u",
		pi, POKE(pi).nickname.c_str(),
		mi, moveName.c_str(),
		POKE(pi).movePP[mi], POKE(pi).maxPP[mi]
	);
}

static void _PokemonTeam_Own_Update(int pi, struct Pokemon* data) {
	PARTY_RANGE(pi)

	if(data == nullptr || !data->box.hasSpecies) {
		POKE(pi).raw = nullptr;
		POKE(pi).nickname = "<null>";

		LOG.Info(frameNum,"PokemonTeam_Own_Update: Pokemon %d is empty", pi);
		return;
	}

	DATA.pokemon[pi].raw = data;

	// load pokemon parameters
	POKE(pi).speciesId = POKE_PARA(data, MON_DATA_SPECIES);
	POKE(pi).heldItemId = POKE_PARA(data, MON_DATA_HELD_ITEM);
	POKE(pi).level = POKE_PARA(data, MON_DATA_LEVEL);

	const auto abilityNum = POKE_PARA(data, MON_DATA_ABILITY_NUM);
	POKE(pi).abilityId = eme.gSpeciesInfo[POKE(pi).speciesId].abilities[abilityNum & 1];

	POKE(pi).typeIds[0] = eme.gSpeciesInfo[POKE(pi).speciesId].types[0];
	POKE(pi).typeIds[1] = eme.gSpeciesInfo[POKE(pi).speciesId].types[1];

	#ifdef ENABLE_SDL2
		// update various strings
		auto _str = EmeraldStringToUTF8(eme.gAbilityNames[POKE(pi).abilityId]);
		POKE(pi).abilityName = std::string(_str.cbegin(), _str.cend());

		if(POKE(pi).heldItemId == ITEM_NONE) {
			POKE(pi).heldItemName = "none";		// GRRRR

		} else {
			_str = EmeraldStringToUTF8(eme.gItems[POKE(pi).heldItemId].name);
			POKE(pi).heldItemName = std::string(_str.cbegin(), _str.cend());
		}
	#endif

	// update Pokemon nickname (can be species or nickname!)
	u8 nickname[POKEMON_NAME_LENGTH + 1];
	eme.GetMonData3(data, MON_DATA_NICKNAME, nickname);
	eme.StringGet_Nickname(nickname);
	_str = EmeraldStringToUTF8(nickname);
	POKE(pi).nickname = std::string(_str.cbegin(), _str.cend());

	// update all moves
	for(uint8_t i = 0; i < MAX_MON_MOVES; i++) {
		_PokemonTeam_Own_UpdateMove(pi, i, data);
	}

	LOG.Info(frameNum,
		"PokemonTeam_Own_Update: Pokemon %d called %s",
		pi, POKE(pi).nickname.c_str()
	);
}

const struct DLL_Events dll_events = {
	.SetGameState = SetGameState,
	.Pokemon_Got = _Pokemon_Got,
	.PokemonTeam_Own_Update = _PokemonTeam_Own_Update,
	.PokemonTeam_Own_LevelUp = _PokemonTeam_Own_LevelUp,
	.PokemonTeam_Own_Status = _PokemonTeam_Own_Status,
	.PokemonTeam_Own_UpdatePP = _PokemonTeam_Own_UpdatePP,
	.PokemonTeam_Own_UpdateMove = _PokemonTeam_Own_UpdateMove,
};

