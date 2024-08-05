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

	int GenerateInt(int min, int max)
	{
		disRange1 = std::uniform_int_distribution<>(min, max);
		return disRange1(gen);
	}

	int GenerateInt(int range1Min, int range1Max, int range2Min, int range2Max)
	{
		int totalRangeSize = (range1Max - range1Min) + (range2Max - range2Min);
		disRange1 = std::uniform_int_distribution<>(range1Min, range1Max);
		disRange2 = std::uniform_int_distribution<>(range2Min, range2Max);

		int choice = std::uniform_int_distribution<>(0, totalRangeSize)(gen);
		if (choice <= (range1Max - range1Min))
		{
			return disRange1(gen);
		}
		return disRange2(gen);
	}

private:
	RandomGenerator() : gen(rd())
	{
	}

	std::random_device rd;
	std::mt19937 gen;
	std::uniform_int_distribution<> disRange1;
	std::uniform_int_distribution<> disRange2;
};
