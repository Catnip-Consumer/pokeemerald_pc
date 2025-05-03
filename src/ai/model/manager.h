#pragma once

#include <vector>
#include <cassert>

#include <mlpack/core.hpp>
#include <mlpack/methods/ann/ffn.hpp>
#include <mlpack/methods/ann/loss_functions/mean_squared_error.hpp>
#include <mlpack/methods/ann/init_rules/random_init.hpp>
#include <mlpack/methods/ann/layer/layer.hpp>

#include <ai/main.h>

// these will cause issues later!
#undef min
#undef max
#undef abs

using namespace mlpack;

/* Define training typenames for easier access */
using ActorNetwork = FFN<>;
using State = arma::colvec;
using Action = arma::Row<size_t>;
using Reward = double;

#define MODEL_STATE_SIZE (4+(PARTY_SIZE*(7+(2*MAX_MON_MOVES))))
#define MODEL_ACTION_SIZE 10
#define MODEL_STEP_COUNT 10
#define MODEL_BATCH_SIZE 256
#define MODEL_GAMMA 0.98

struct Experience {
	State state;
	State nextState;
	Action action;
	Reward reward;
	Reward interest;
};

using ReplayBuffer = std::vector<Experience>;

// Shared across threads
extern ReplayBuffer replayBuffers[CONCURRENT_AGENTS];
extern ActorNetwork agentModelCopies[CONCURRENT_AGENTS];

inline constexpr double GetEpsilonGreedyChance(size_t generation) {
	if(generation <= 10) {
		return 1.0;
	}

	return std::max(0.025, 0.3 * std::pow(0.99, generation));
}

#define SIMULATION_SAVES_COUNT 1
#define SIMULATION_FRAMECOUNT ((5*60*60*60) - 60)

// Save game is loaded to flash buffer and is read-only for agents.
extern uint8_t flash[SIMULATION_SAVES_COUNT][sizeof(FLASH_BASE)];
