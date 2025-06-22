#pragma once

#include <vector>
#include <memory>
#include <span>
#include <algorithm>
#include "Vector2.h"
#include "DisplayData.h"
#include "Splatter.h"
#include "ErrorHandling.h"

namespace RainEngine {

// Modern RainDrop class with C++20 features and improved performance
class RainDrop {
public:
    // Constructor with dependency injection
    explicit RainDrop(int windDirectionFactor, DisplayData* displayData) noexcept;
    
    // Rule of five with modern defaults
    ~RainDrop() noexcept = default;
    RainDrop(const RainDrop&) = delete;
    RainDrop& operator=(const RainDrop&) = delete;
    RainDrop(RainDrop&&) = default;
    RainDrop& operator=(RainDrop&&) = default;

    // Core functionality with const correctness and noexcept
    void UpdatePosition(float deltaTime) noexcept;
    void Draw(ID2D1DeviceContext* deviceContext) const noexcept;
    
    // State queries
    [[nodiscard]] constexpr bool DidTouchGround() const noexcept { return hasGroundContact_; }
    [[nodiscard]] constexpr bool IsAlive() const noexcept { return !isDead_; }
    [[nodiscard]] constexpr bool IsReadyForErase() const noexcept { return isDead_; }

    // Position and physics accessors
    [[nodiscard]] constexpr const Vector2& GetPosition() const noexcept { return position_; }
    [[nodiscard]] constexpr const Vector2& GetVelocity() const noexcept { return velocity_; }
    [[nodiscard]] constexpr float GetRadius() const noexcept { return radius_; }
    [[nodiscard]] constexpr float GetTrailLength() const noexcept { return trailLength_; }

    // Splatter information
    [[nodiscard]] constexpr size_t GetSplatterCount() const noexcept { return splatters_.size(); }
    [[nodiscard]] std::span<const std::unique_ptr<Splatter>> GetSplatters() const noexcept { return splatters_; }

private:
    // Physics constants with C++20 constexpr improvements
    static constexpr float TERMINAL_VELOCITY_Y = 1000.0f; // pixels per second
    static constexpr float WIND_MULTIPLIER = 75.0f;       // pixels per second
    static constexpr float SPLATTER_STARTING_VELOCITY = 200.0f;
    static constexpr int MAX_SPLATTERS_PER_DROP = 3;
    static constexpr int MAX_SPLATTER_FRAME_COUNT = 50;
    static constexpr float PI = std::numbers::pi_v<float>;

    // Physics state
    Vector2 position_{};
    Vector2 velocity_{};
    float radius_{1.0f};
    float trailLength_{50.0f};
    
    // Simulation state
    bool hasGroundContact_{false};
    bool isDead_{false};
    int windDirectionFactor_{0};
    int splatterFrameCount_{0};

    // Dependencies (non-owning)
    DisplayData* displayData_{nullptr};

    // Splatters with modern container management
    std::vector<std::unique_ptr<Splatter>> splatters_;

    // Private methods with improved naming
    void Initialize() noexcept;
    void CreateSplatters() noexcept;
    [[nodiscard]] bool ShouldDrawTrail(const Vector2& trailStart) const noexcept;
    [[nodiscard]] Vector2 CalculateTrailStart() const noexcept;
    void UpdateSplatters(float deltaTime) noexcept;
    void DrawTrail(ID2D1DeviceContext* deviceContext, const Vector2& trailStart) const noexcept;
    void DrawSplatters(ID2D1DeviceContext* deviceContext) const noexcept;
};

} // namespace RainEngine