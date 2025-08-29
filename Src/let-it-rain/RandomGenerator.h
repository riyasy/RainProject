#pragma once

#include <random>
#include <mutex>
#include <shared_mutex>
#include <concepts>
#include <type_traits>

namespace RainEngine {

// Modern thread-safe random number generator using C++20 concepts
class RandomGenerator {
public:
    // Delete copy and move operations for singleton
    RandomGenerator(const RandomGenerator&) = delete;
    RandomGenerator& operator=(const RandomGenerator&) = delete;
    RandomGenerator(RandomGenerator&&) = delete;
    RandomGenerator& operator=(RandomGenerator&&) = delete;

    // Thread-safe singleton instance using std::call_once
    [[nodiscard]] static RandomGenerator& GetInstance() noexcept {
        static RandomGenerator instance;
        return instance;
    }

    // Modern templated interface with concepts (C++20)
    template<std::integral T>
    [[nodiscard]] T GenerateInt(T min, T max) noexcept {
        std::shared_lock lock(mutex_);
        return std::uniform_int_distribution<T>{min, max}(generator_);
    }

    template<std::floating_point T>
    [[nodiscard]] T GenerateFloat(T min, T max) noexcept {
        std::shared_lock lock(mutex_);
        return std::uniform_real_distribution<T>{min, max}(generator_);
    }

    // Specialized method for dual range generation (preserving original functionality)
    template<std::integral T>
    [[nodiscard]] T GenerateInt(T range1Min, T range1Max, T range2Min, T range2Max) noexcept {
        std::shared_lock lock(mutex_);
        
        const auto range1Size = range1Max - range1Min + 1;
        const auto range2Size = range2Max - range2Min + 1;
        const auto totalSize = range1Size + range2Size;
        
        const auto choice = std::uniform_int_distribution<T>{0, totalSize - 1}(generator_);
        
        if (choice < range1Size) {
            return std::uniform_int_distribution<T>{range1Min, range1Max}(generator_);
        } else {
            return std::uniform_int_distribution<T>{range2Min, range2Max}(generator_);
        }
    }

    // Boolean generation
    [[nodiscard]] bool GenerateBool(double probability = 0.5) noexcept {
        std::shared_lock lock(mutex_);
        return std::bernoulli_distribution{probability}(generator_);
    }

    // Normal distribution
    template<std::floating_point T>
    [[nodiscard]] T GenerateNormal(T mean = T{0}, T stddev = T{1}) noexcept {
        std::shared_lock lock(mutex_);
        return std::normal_distribution<T>{mean, stddev}(generator_);
    }

    // Exponential distribution
    template<std::floating_point T>
    [[nodiscard]] T GenerateExponential(T lambda = T{1}) noexcept {
        std::shared_lock lock(mutex_);
        return std::exponential_distribution<T>{lambda}(generator_);
    }

    // Generate random element from container
    template<typename Container>
    [[nodiscard]] auto& GenerateChoice(Container& container) noexcept 
        requires requires { container.size(); container.begin(); } {
        
        std::shared_lock lock(mutex_);
        auto dist = std::uniform_int_distribution<size_t>{0, container.size() - 1};
        auto it = container.begin();
        std::advance(it, dist(generator_));
        return *it;
    }

    // Seed the generator (thread-safe)
    void Seed(uint_fast32_t seed) noexcept {
        std::unique_lock lock(mutex_);
        generator_.seed(seed);
    }

    // Seed with random device
    void SeedWithRandomDevice() noexcept {
        std::unique_lock lock(mutex_);
        generator_.seed(randomDevice_());
    }

    // Get engine for advanced usage (not recommended for general use)
    template<typename Func>
    decltype(auto) WithEngine(Func&& func) {
        std::unique_lock lock(mutex_);
        return func(generator_);
    }

private:
    RandomGenerator() noexcept : generator_(randomDevice_()) {
        // Constructor initializes with random device seed
    }

    std::random_device randomDevice_;
    std::mt19937 generator_;
    mutable std::shared_mutex mutex_; // Allows multiple concurrent reads
};

// Convenience functions for common use cases
namespace Random {
    [[nodiscard]] inline int Int(int min, int max) noexcept {
        return RandomGenerator::GetInstance().GenerateInt(min, max);
    }

    [[nodiscard]] inline float Float(float min = 0.0f, float max = 1.0f) noexcept {
        return RandomGenerator::GetInstance().GenerateFloat(min, max);
    }

    [[nodiscard]] inline double Double(double min = 0.0, double max = 1.0) noexcept {
        return RandomGenerator::GetInstance().GenerateFloat(min, max);
    }

    [[nodiscard]] inline bool Bool(double probability = 0.5) noexcept {
        return RandomGenerator::GetInstance().GenerateBool(probability);
    }

    [[nodiscard]] inline float Normal(float mean = 0.0f, float stddev = 1.0f) noexcept {
        return RandomGenerator::GetInstance().GenerateNormal(mean, stddev);
    }

    template<typename Container>
    [[nodiscard]] inline auto& Choice(Container& container) noexcept {
        return RandomGenerator::GetInstance().GenerateChoice(container);
    }
}

} // namespace RainEngine

// Type alias for backward compatibility during transition
using RandomGenerator = RainEngine::RandomGenerator;