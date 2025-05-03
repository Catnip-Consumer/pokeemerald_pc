#include <vector>
#include <random>
#include <algorithm>
#include <stdexcept>
#include <cassert>

class WeightedSampler {
public:
	WeightedSampler() :
		cumulativeWeights(),
		totalWeight(0.0) {
	}

	WeightedSampler(const std::vector<double>& input) : WeightedSampler() {
		this->setWeights(input);
	}

	WeightedSampler(std::vector<double>::const_iterator begin, std::vector<double>::const_iterator end) : WeightedSampler() {
		this->setWeights(begin, end);
	}

	bool empty() const noexcept {
		return totalWeight <= 0.0;
	}

	size_t size() const noexcept {
		return cumulativeWeights.size();
	}

	size_t max_size() const noexcept {
		return cumulativeWeights.max_size();
	}

	void resize(size_t count) {
		return cumulativeWeights.resize(count, 0.0);
	}

	WeightedSampler& operator=(const std::vector<double>& input) {
		this->setWeights(input);
		return *this;
	}

	void setWeights(const std::vector<double>& input) {
		this->setWeights(input.cbegin(), input.cend());
	}

	void setWeights(std::vector<double>::const_iterator begin, std::vector<double>::const_iterator end) {
		cumulativeWeights.resize(std::distance(begin, end));
		totalWeight = 0.0;

		/* Sets initial weights for the vector */
		size_t i = 0;
		while(begin != end) {
			totalWeight += *begin;
			cumulativeWeights[i++] = totalWeight;
			++begin;
		}
	}

	void set(size_t index, double weight) {
		if(index >= cumulativeWeights.size()) {
			throw std::runtime_error("Trying to set index "+ std::to_string(index) +" which is out of bounds.");
		}

		if(cumulativeWeights[index] != 0) {
			throw std::runtime_error("Weight at index "+ std::to_string(index) +" was already set to something.");
		}

		totalWeight += weight;
		cumulativeWeights[index] = totalWeight;
	}

	size_t operator()(std::mt19937 random) {
		return this->sample(random);
	}

	size_t sample(std::mt19937 random) {
		/* Generate distribution and read its next value */
		std::uniform_real_distribution<double> dis(0.0, totalWeight);
		double r = dis(random);

		/* Get the iterator for this position */
		auto it = std::upper_bound(cumulativeWeights.begin(), cumulativeWeights.end(), r);
		assert(it != cumulativeWeights.end());
		return std::distance(cumulativeWeights.begin(), it);
	}

	void ignore(size_t index) {
		// Using raw data isn't exactly safe... but it sure is fast!
		double* rawData = cumulativeWeights.data();
		double diff = 0.0;

		if(index == 0) {
			// first element difference is always element size
			diff = rawData[index];

		} else {
			// calculate the difference between the current and previous iterator position
			diff = rawData[index] - rawData[index - 1];
		}

		// "Collapse" the remaining space into the ignored item
		totalWeight -= diff;

		for(;index < cumulativeWeights.size();index++){
			rawData[index] -= diff;
		}

		if (totalWeight <= 0.0) {
			throw std::runtime_error("Collapse caused totalWeight to go to 0 or under!");
		}
	}

private:
	std::vector<double> cumulativeWeights;
	double totalWeight;
};
