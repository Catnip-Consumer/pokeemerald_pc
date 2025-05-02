#include <iostream>
#include <thread>
#include <fstream>

#include <ai/model-manager.h>

static std::mt19937 mt19937{ std::random_device{}() };

std::mutex agentMutex;
AgentDataStruct agentData;
volatile AgentState agentState;
volatile size_t generation = 0;

ReplayBuffer replayBuffers[CONCURRENT_AGENTS];
ActorNetwork agentModelCopies[CONCURRENT_AGENTS];

static void createDllCopy(size_t num) {
#ifdef _WIN32
	/* Delete the old DLL (should be able to without deletion but I can't be bothered rn) */
	const auto target = getExecutableDir() / "_dll" / ("libemerald_" + std::to_string(num) + ".dll");

	if(std::filesystem::exists(target)) {
		std::filesystem::remove(target);
	}

	std::filesystem::copy_file(getExecutableDir() / "libemerald.dll", target);
#endif
}

uint8_t flash[sizeof(FLASH_BASE)];

static void ReadSaveFile() {
	// fill flash buffer with 0xFF and read contents
	memset(flash, 0xFF, sizeof(flash));
	std::ifstream savefile;

	try {
		const auto savePath = getExecutableDir() / "emerald-ai.sav";
		savefile = std::ifstream(savePath, std::ios::binary);

		// get file size
		savefile.seekg(0, std::ios::end);
		const auto size = savefile.tellg();
		savefile.seekg(0, std::ios::beg);

		// read from file
		const auto readSize = min((std::streampos) size, (std::streampos) sizeof(flash));
		savefile.read(reinterpret_cast<char*>(flash), size);

		std::cout << "Read " << size << " bytes from " << savePath << std::endl;

	} catch (std::exception*) {
		// assume the file was not found
	}

	// close the file
	if (savefile.is_open()) {
		savefile.close();
	}
}

static double TrainStep(std::vector<Experience>& batch, ActorNetwork& model, size_t maxEpochs = 1) {
	double totalLoss = 0.0;
	arma::mat input;  // Each column is a state
	arma::mat target; // Each column is a target Q vector

	const size_t stateSize = batch[0].state.n_rows;

	input.set_size(stateSize, MODEL_BATCH_SIZE);
	target.set_size(MODEL_ACTION_SIZE, MODEL_BATCH_SIZE);

	for (size_t i = 0; i < MODEL_BATCH_SIZE; ++i) {
		const Experience& e = batch[i];

		input.col(i) = e.state;

		arma::colvec predictedQ(MODEL_ACTION_SIZE);
		model.Predict(e.state, predictedQ);

		// Save original prediction for loss
		arma::colvec lossVec = predictedQ;

		arma::colvec nextQ(MODEL_ACTION_SIZE);
		model.Predict(e.nextState, nextQ);

		if (!arma::is_finite(e.reward)) {
			throw std::runtime_error("NaN or Inf in reward!");
		}

		if (!arma::is_finite(nextQ.max())) {
			throw std::runtime_error("NaN or Inf in nextQ max!");
		}

		// Calculate the target Q-values based on the reward and max future Q-value
		double qTarget = e.reward + (MODEL_GAMMA * nextQ.max());

		// Now update qValues for the actions in e.action (which could be multiple actions)
		// Loop through all actions in e.action and update their Q-value
		lossVec.elem(e.action != 0).fill(qTarget);

		// Set the target values for this experience
		target.col(i) = lossVec;

		// Mean Squared Error
		double loss = arma::accu(arma::square(predictedQ - lossVec));
		totalLoss += loss;
	}

	if (!target.is_finite()) {
		throw std::runtime_error("Target matrix contains NaN or Inf!");
	}

	ens::Adam optimizer(0.001, MODEL_BATCH_SIZE, 0.9, 0.999, 1e-8, maxEpochs, 1e-5, true);
	model.Train(input, target, optimizer);

	return totalLoss / MODEL_BATCH_SIZE;
}

inline static void Train(ActorNetwork& model) {
	auto start = std::chrono::high_resolution_clock::now();

	/* Grab training data buffer */
	ReplayBuffer data;

	for(auto& buffer : replayBuffers) {
		/* Append each agent buffer to shared buffer */
		data.insert(data.end(), buffer.begin(), buffer.end());
		buffer.clear();
	}

	if (data.size() < MODEL_BATCH_SIZE) {
		throw std::runtime_error(std::to_string(data.size()) +" is not enough samples to train on!");
	}

	/* Generate an array of weights based on the interest in training data */
	std::vector<double> weights(data.size());
	std::transform(
		data.begin(), data.end(), weights.begin(),
		[](const Experience& e) {
			return e.interest + 1e-5;
		 }
	);

	/* Create a distribution of the weights so they are picked randomly but with bias */
	std::discrete_distribution<> dist(weights.begin(), weights.end());

    double totalQValue = 0;
	double totalLoss = 0;

	for (size_t i = 0; i < MODEL_STEP_COUNT; ++i) {
		/* Sample a mini batch for the training step */
		std::vector<Experience> miniBatch;
		miniBatch.reserve(MODEL_BATCH_SIZE);
		std::generate_n(
			std::back_inserter(miniBatch), MODEL_BATCH_SIZE, [&]() {
				return data[dist(mt19937)];
		});

		/* Run the training on the model */
		totalLoss += TrainStep(miniBatch, model);
	}

	/* Record how long training took */
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = end - start;

    /* Average the loss and Q-values */
    double avgLoss = totalLoss / MODEL_STEP_COUNT;
    double avgQValue = totalQValue / MODEL_STEP_COUNT;

	std::cout << "Training in gen " << generation << " took " << (size_t) elapsed.count() << " ms... ";
    std::cout << "Avg Loss: " << avgLoss << ", Mean Q-Value: " << avgQValue << ", Epsilon: " << GetEpsilonGreedyChance(generation);
    std::cout << std::endl;
}

static void runGeneration(ActorNetwork& model) {
	/* Print out generation info */
	generation = generation + 1;
	std::cout << "Initializing generation " << generation << "..." << std::endl;

	{
		/* Tell agents to wait for sync */
		std::lock_guard<std::mutex> lock(agentMutex);
		agentState = AgentState::WAIT_SYNC;
		agentData.agentsCounter = CONCURRENT_AGENTS;
	}

	/* Start each agent thread */
	std::thread agent[CONCURRENT_AGENTS];

	for(int i = 0; i < CONCURRENT_AGENTS; i++) {
		agent[i] = std::thread(runAgent, generation, i);
	}

	/* While agents are initializing, create local copies of the model for each agent */
	for(int i = 0; i < CONCURRENT_AGENTS; i++) {
		agentModelCopies[i] = model;
	}

	/* Wait until each agent has initialized and are waiting for sync */
	std::cout << "Waiting for agents to sync..." << std::endl;

	while(agentState != AgentState::EXIT) {
		std::this_thread::sleep_for(std::chrono::milliseconds(3));

		if(agentData.agentsCounter <= 0) {
			break;
		}
	}

	{
		/* Agents are now synced, allow them to run their code */
		std::lock_guard<std::mutex> lock(agentMutex);
		agentState = AgentState::RUNNING;
		agentData.agentsCounter = CONCURRENT_AGENTS;
	}

	std::cout << "Running " << CONCURRENT_AGENTS << " agents for generation " << generation << "..." << std::endl;

	while(agentState != AgentState::EXIT) {
		/* Agents are running their code. just wait til they are done... */
		std::this_thread::sleep_for(std::chrono::milliseconds(20));

		if(agentData.agentsCounter <= 0) {
			break;
		}
	}

	/* Simulation has finished.. */
	for(int i = 0; i < CONCURRENT_AGENTS; i++) {
		agent[i].join();
	}

	/* Run training function */
	std::cout << "Agents are done running. Train model..." << std::endl;
	Train(model);
}

void runModelThread() {
	/*
	 * Set our affinity to core0. No agent should have permission to run on core0.
	 * This ensures that the model can update itself in relative peace
	 */
	auto threadHandle = getCurrentThreadHandle();
	setThreadAffinity(threadHandle, true);
	closeThreadHandle(threadHandle);

	ReadSaveFile();

	/*
	 * Create a directory for DLL's to be dumped on. This method makes sure that Windows thinks we're
	 * opening many DLL's, which ensures thread safety. This is dumb but an easy solution to a hard problem.
	 */
	#ifdef _WIN32
		std::filesystem::create_directory(getExecutableDir() / "_dll");

		// Now copy the DLL for each agent
		for(int i = 0; i < CONCURRENT_AGENTS; i++) {
			createDllCopy(i);
		}
	#endif

	/* Generate our initial model */
	auto model = std::make_shared<ActorNetwork>();
	model->Add<Linear>(200);
	model->Add<ReLU>();
	model->Add<Linear>(MODEL_ACTION_SIZE);
	model->Add<Sigmoid>();

	/* keep running generations until an exit signal was raised. */
	while(agentState != AgentState::EXIT) {
		runGeneration(*model);
	}

	/* Do some final cleanup here... */
	std::cout << "Simulation ending... Do cleanup." << std::endl;
}
