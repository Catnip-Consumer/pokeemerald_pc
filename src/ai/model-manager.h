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

#define SIMULATION_FRAMECOUNT ((6*60*60*60) - 1)
#define EPSILON_GREEDY_CHANCE 0.3

#define MODEL_STATE_SIZE 84
#define MODEL_ACTION_SIZE 10
#define MODEL_STEP_COUNT 20
#define MODEL_BATCH_SIZE 4096

struct Experience {
	State state;
	State nextState;
	Action action;
	Reward reward;
};

using ReplayBuffer = std::vector<Experience>;

// Shared across threads
extern ReplayBuffer replayBuffers[CONCURRENT_AGENTS];
extern ActorNetwork agentModelCopies[CONCURRENT_AGENTS];
