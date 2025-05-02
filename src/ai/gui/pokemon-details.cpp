#include <cassert>
#include <cmath>
#include <iostream>

#include <ai/gui/gui.h>
#include <ai/main.h>

extern "C" {
	#include <pokemon.h>
	#include <battle.h>
	#include <string_util.h>

	// these will cause issues later!
	#undef min
	#undef max
	#undef abs
}

#define PARTY_GET(idx) agentData.data[agentId].pokemon[idx]

#define PARTY_ITER(var)							\
	ImGui::TableNextRow();						\
	for(size_t i = 0; i < PARTY_SIZE; i++) {	\
		const auto var = PARTY_GET(i);			\
		if(var.raw == nullptr) {				\
			continue;							\
		}

#define STATUS_CHECK(bits, str)			\
	if((poke->status & bits) != 0) {	\
		if(!first) st += ", ";			\
		st += str; first = false;		\
	}

const std::string getPokemonPrimaryStatusString(struct Pokemon* poke) {
	if(poke->hp == 0) {
		return "(FAINTED)";
	}

	std::string st;
	bool first = true;

	STATUS_CHECK(STATUS1_SLEEP,				"SLEEPING")
	STATUS_CHECK(STATUS1_FREEZE,			"FROZEN")
	STATUS_CHECK(STATUS1_BURN,				"BURNED")
	STATUS_CHECK(STATUS1_PARALYSIS,			"PARALYZED")
	STATUS_CHECK(STATUS1_POISON,			"POISONED")
	STATUS_CHECK(STATUS1_TOXIC_POISON,		"BADLY POISONED")
	return st.length() == 0 ? "" : "(" + st + ")";
}

void agentPartyWindow(size_t agentId) {
	if(!ImGui::Begin(WDNAME(AGENT_PARTY), nullptr) || !agentData.data[agentId].active) {
		ImGui::End();
		return;
	}

	if(!ImGui::BeginTable("Partybox", 6, ImGuiTableFlags_ScrollY|ImGuiTableFlags_BordersInnerV)) {
		ImGui::End();
		return;
	}

	// Set table header cols
	auto colflags = ImGuiTableColumnFlags_NoReorder|ImGuiTableColumnFlags_NoResize|ImGuiTableColumnFlags_NoClip;

	for(size_t i = 0; i < PARTY_SIZE; i++) {
		ImGui::TableSetupColumn(("c"+ std::to_string(i)).c_str(), colflags);
	}

	// Draw pokemon names
	PARTY_ITER(pokemon)
		ImGui::TableNextColumn();
		ImGui::Text(
			"%s (%u) level %u",
			pokemon.nickname.c_str(),
			pokemon.speciesId,
			(uint16_t) pokemon.level
		);
	}

	// Draw item and ability
	PARTY_ITER(pokemon)
		ImGui::TableNextColumn();
		ImGui::Text(
			"Holding: %s | Ability: %s",
			pokemon.heldItemName.c_str(),
			pokemon.abilityName.c_str()
		);
	}

	// Draw type
	PARTY_ITER(pokemon)
		ImGui::TableNextColumn();
		ImGui::Text(
			"Type: %u %u",
			pokemon.typeIds[0],
			pokemon.typeIds[1]
		);
	}

	// Draw pokemon HP
	PARTY_ITER(pokemon)
		std::string status = getPokemonPrimaryStatusString(pokemon.raw);

		ImGui::TableNextColumn();
		ImGui::Text(
			"%u/%u HP (%d%% left) %s",
			pokemon.raw->hp,
			pokemon.raw->maxHP,
			(uint16_t) roundf(100.f * (pokemon.raw->hp / (float) pokemon.raw->maxHP)),
			status.c_str()
		);
	}

	// Draw Stats 1
	PARTY_ITER(pokemon)
		ImGui::TableNextColumn();
		ImGui::Text(
			"%u Atk | %u Sp. Atk",
			pokemon.raw->attack,
			pokemon.raw->spAttack
		);
	}

	// Draw Stats 2
	PARTY_ITER(pokemon)
		ImGui::TableNextColumn();
		ImGui::Text(
			"%u Def | %u Sp. Def | %u Spd",
			pokemon.raw->defense,
			pokemon.raw->spDefense,
			pokemon.raw->speed
		);
	}

	// Draw move slots
	for(size_t m = 0; m < MAX_MON_MOVES; m++) {
		PARTY_ITER(pokemon)
			ImGui::TableNextColumn();
			ImGui::Text(
				"%s %u / %u PP",
				pokemon.moveName[m].c_str(),
				pokemon.movePP[m],
				pokemon.maxPP[m]
			);
		}
	}

	ImGui::EndTable();
	ImGui::End();
}
