#pragma once

#include <vector>
#include <memory>
#include <concepts>
#include <algorithm>
#ifdef __cpp_lib_span
    #include <span>
#endif
#ifdef __cpp_lib_execution
    #include <execution>
#endif
#include "Vector2.h"
#include "DisplayData.h"
#include "ErrorHandling.h"

namespace RainEngine {

// Concept for particle types
template<typename T>
concept Particle = requires(T t, float deltaTime, ID2D1DeviceContext* dc) {
    { t.UpdatePosition(deltaTime) } -> std::same_as<void>;
    { t.Draw(dc) } -> std::same_as<void>;
    { t.IsAlive() } -> std::same_as<bool>;
};

// Modern particle system base class using C++20 concepts and ranges
template<Particle ParticleType>
class ParticleSystem {
public:
    using ParticlePtr = std::unique_ptr<ParticleType>;
    using Container = std::vector<ParticlePtr>;

    explicit ParticleSystem(size_t reserveCapacity = 1000) {
        particles_.reserve(reserveCapacity);
    }

    virtual ~ParticleSystem() = default;

    // Non-copyable but movable
    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;
    ParticleSystem(ParticleSystem&&) = default;
    ParticleSystem& operator=(ParticleSystem&&) = default;

    // Modern interface with const correctness
    [[nodiscard]] constexpr size_t GetParticleCount() const noexcept { return particles_.size(); }
    [[nodiscard]] constexpr bool IsEmpty() const noexcept { return particles_.empty(); }
    [[nodiscard]] constexpr size_t GetCapacity() const noexcept { return particles_.capacity(); }

    // Span interface for direct access (C++20) with fallback
    #ifdef __cpp_lib_span
    [[nodiscard]] std::span<ParticlePtr> GetParticles() noexcept { return particles_; }
    [[nodiscard]] std::span<const ParticlePtr> GetParticles() const noexcept { return particles_; }
    #else
    // Fallback for compilers without std::span
    [[nodiscard]] ParticlePtr* GetParticlesRaw() noexcept { return particles_.data(); }
    [[nodiscard]] const ParticlePtr* GetParticlesRaw() const noexcept { return particles_.data(); }
    #endif

    // Update all particles with optional parallelization
    void UpdateParticles(float deltaTime, bool useParallelExecution = true) noexcept {
        if (particles_.empty()) return;

        #ifdef __cpp_lib_execution
        if (useParallelExecution && particles_.size() > 100) {
            // Use parallel execution for large particle counts
            std::for_each(std::execution::par_unseq, 
                         particles_.begin(), particles_.end(),
                         [deltaTime](const auto& particle) {
                             if (particle && particle->IsAlive()) {
                                 particle->UpdatePosition(deltaTime);
                             }
                         });
        } else
        #endif
        {
            // Sequential execution for smaller counts or when parallelization is disabled
            for (const auto& particle : particles_) {
                if (particle && particle->IsAlive()) {
                    particle->UpdatePosition(deltaTime);
                }
            }
        }
    }

    // Remove dead particles using modern algorithms
    void RemoveDeadParticles() noexcept {
        // Use remove_if with modern lambda and smart pointer handling
        auto newEnd = std::remove_if(particles_.begin(), particles_.end(),
            [](const auto& particle) {
                return !particle || !particle->IsAlive();
            });
        
        particles_.erase(newEnd, particles_.end());
    }

    // Draw all particles
    void DrawParticles(ID2D1DeviceContext* deviceContext) const noexcept {
        if (!deviceContext || particles_.empty()) return;

        for (const auto& particle : particles_) {
            if (particle && particle->IsAlive()) {
                particle->Draw(deviceContext);
            }
        }
    }

    // Add particle with perfect forwarding
    template<typename... Args>
    void EmplaceParticle(Args&&... args) {
        try {
            particles_.emplace_back(std::make_unique<ParticleType>(std::forward<Args>(args)...));
        } catch (const std::bad_alloc&) {
            // Handle allocation failure gracefully
            // Could implement a fallback or logging here
        }
    }

    // Batch add particles with exception safety
    [[nodiscard]] Result AddParticles(size_t count, auto&& particleFactory) noexcept {
        try {
            particles_.reserve(particles_.size() + count);
            
            for (size_t i = 0; i < count; ++i) {
                auto particle = particleFactory();
                if (particle) {
                    particles_.emplace_back(std::move(particle));
                }
            }
            
            return Result::Success();
        } catch (const std::bad_alloc&) {
            return Result::Error(Result::ErrorCode::ResourceAllocationFailed,
                               "Failed to allocate memory for particles");
        } catch (const std::exception& e) {
            return Result::Error(Result::ErrorCode::ResourceAllocationFailed, e.what());
        } catch (...) {
            return Result::Error(Result::ErrorCode::ResourceAllocationFailed,
                               "Unknown error in AddParticles");
        }
    }

    // Clear all particles
    void Clear() noexcept {
        particles_.clear();
    }

    // Reserve capacity to avoid reallocations
    void Reserve(size_t capacity) {
        particles_.reserve(capacity);
    }

    // Shrink container to fit current size
    void ShrinkToFit() noexcept {
        try {
            particles_.shrink_to_fit();
        } catch (...) {
            // Ignore shrink_to_fit failures as they're not critical
        }
    }

protected:
    Container particles_;
};

// Type aliases for common particle systems
class RainDrop;
class SnowFlake;

using RainParticleSystem = ParticleSystem<RainDrop>;
using SnowParticleSystem = ParticleSystem<SnowFlake>;

} // namespace RainEngine