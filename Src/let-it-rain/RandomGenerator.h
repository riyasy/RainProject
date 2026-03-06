#pragma once

#include <random>

class RandomGenerator
{
public:
	// Delete copy constructor and assignment operator to prevent copying
	RandomGenerator(const RandomGenerator&) = delete;
	RandomGenerator& operator=(const RandomGenerator&) = delete;

	// Static method to get the singleton instance
	static RandomGenerator& GetInstance()
	{
		static RandomGenerator instance;
		return instance;
	}

	int GenerateInt(const int min, const int max)
	{
		return std::uniform_int_distribution<>(min, max)(gen);
	}

	int GenerateInt(const int range1Min, const int range1Max, const int range2Min, const int range2Max)
	{
		const int range1Size = range1Max - range1Min;
		const int range2Size = range2Max - range2Min;
		const int totalRangeSize = range1Size + range2Size;
		const int choice = std::uniform_int_distribution<>(0, totalRangeSize)(gen);
		if (choice <= range1Size)
		{
			return std::uniform_int_distribution<>(range1Min, range1Max)(gen);
		}
		return std::uniform_int_distribution<>(range2Min, range2Max)(gen);
	}

	// Generate a floating point number in [min, max)
	float GenerateFloat(const float min, const float max)
	{
		return std::uniform_real_distribution<float>(min, max)(gen);
	}

private:
	RandomGenerator() : gen(rd())
	{
	}

	std::random_device rd;
	std::mt19937 gen;
};
