#pragma once

#include <cmath>
#ifdef __cpp_lib_math_constants
    #include <numbers>
#endif
#ifdef __cpp_lib_format
    #include <format>
#endif
#include <compare>
#include <algorithm>
#include <iostream>
#include <string>
#include <cstdio>
#include <d2d1_1.h>

namespace RainEngine {

// Modern C++20 Vector2 class with mathematical operations and formatting support
class Vector2 {
public:
    float x{0.0f};
    float y{0.0f};

    // Constructors with C++20 designated initializers support
    constexpr Vector2() noexcept = default;
    constexpr Vector2(float x_val, float y_val) noexcept : x(x_val), y(y_val) {}
    
    // Copy/move constructors are implicitly defaulted and constexpr

    // Three-way comparison operator (C++20)
    [[nodiscard]] constexpr auto operator<=>(const Vector2& other) const noexcept = default;
    [[nodiscard]] constexpr bool operator==(const Vector2& other) const noexcept = default;

    // Mathematical operations with const correctness and noexcept
    [[nodiscard]] constexpr Vector2 operator+(const Vector2& other) const noexcept {
        return {x + other.x, y + other.y};
    }

    [[nodiscard]] constexpr Vector2 operator-(const Vector2& other) const noexcept {
        return {x - other.x, y - other.y};
    }

    [[nodiscard]] constexpr Vector2 operator*(float scalar) const noexcept {
        return {x * scalar, y * scalar};
    }

    [[nodiscard]] constexpr Vector2 operator/(float scalar) const noexcept {
        return {x / scalar, y / scalar};
    }

    // Compound assignment operators
    constexpr Vector2& operator+=(const Vector2& other) noexcept {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr Vector2& operator-=(const Vector2& other) noexcept {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr Vector2& operator*=(float scalar) noexcept {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Vector2& operator/=(float scalar) noexcept {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    // Mathematical functions with improved naming and const correctness
    [[nodiscard]] constexpr float MagnitudeSquared() const noexcept {
        return x * x + y * y;
    }

    [[nodiscard]] float Magnitude() const noexcept {
        return std::sqrt(MagnitudeSquared());
    }

    [[nodiscard]] constexpr float Dot(const Vector2& other) const noexcept {
        return x * other.x + y * other.y;
    }

    [[nodiscard]] constexpr float Cross(const Vector2& other) const noexcept {
        return x * other.y - y * other.x;
    }

    [[nodiscard]] Vector2 Normalized() const noexcept {
        const auto mag = Magnitude();
        if (mag > 0.0f) {
            return {x / mag, y / mag};
        }
        return {};
    }

    void Normalize() noexcept {
        const auto mag = Magnitude();
        if (mag > 0.0f) {
            x /= mag;
            y /= mag;
        }
    }

    void SetMagnitude(float magnitude) noexcept {
        const auto currentMag = Magnitude();
        if (currentMag > 0.0f) {
            const auto scale = magnitude / currentMag;
            x *= scale;
            y *= scale;
        }
    }

    [[nodiscard]] float DistanceTo(const Vector2& other) const noexcept {
        return (*this - other).Magnitude();
    }

    [[nodiscard]] constexpr float DistanceSquaredTo(const Vector2& other) const noexcept {
        return (*this - other).MagnitudeSquared();
    }

    [[nodiscard]] float AngleTo(const Vector2& other) const noexcept {
        return std::atan2(Cross(other), Dot(other));
    }

    [[nodiscard]] Vector2 Rotated(float angleRadians) const noexcept {
        const auto cosAngle = std::cos(angleRadians);
        const auto sinAngle = std::sin(angleRadians);
        return {
            x * cosAngle - y * sinAngle,
            x * sinAngle + y * cosAngle
        };
    }

    void Rotate(float angleRadians) noexcept {
        *this = Rotated(angleRadians);
    }

    [[nodiscard]] Vector2 Perpendicular() const noexcept {
        return {-y, x};
    }

    [[nodiscard]] Vector2 Lerp(const Vector2& target, float t) const noexcept {
        return *this + (target - *this) * t;
    }

    // Utility methods
    [[nodiscard]] constexpr bool IsZero() const noexcept {
        return x == 0.0f && y == 0.0f;
    }

    [[nodiscard]] bool IsNearZero(float epsilon = 1e-6f) const noexcept {
        return MagnitudeSquared() < epsilon * epsilon;
    }

    void Clamp(float minValue, float maxValue) noexcept {
        x = std::clamp(x, minValue, maxValue);
        y = std::clamp(y, minValue, maxValue);
    }

    [[nodiscard]] Vector2 Clamped(float minValue, float maxValue) const noexcept {
        return {std::clamp(x, minValue, maxValue), std::clamp(y, minValue, maxValue)};
    }

    // DirectX integration
    [[nodiscard]] D2D1_POINT_2F ToD2DPoint() const noexcept {
        return D2D1::Point2F(x, y);
    }

    [[nodiscard]] static constexpr Vector2 FromD2DPoint(const D2D1_POINT_2F& point) noexcept {
        return {point.x, point.y};
    }

    // String representation with fallback for std::format compatibility
    [[nodiscard]] std::string ToString() const {
        #ifdef __cpp_lib_format
            return std::format("Vector2({:.3f}, {:.3f})", x, y);
        #else
            // Fallback for compilers without std::format support
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "Vector2(%.3f, %.3f)", x, y);
            return std::string(buffer);
        #endif
    }

    // Static constants with C++20 std::numbers
    [[nodiscard]] static constexpr Vector2 Zero() noexcept { return {0.0f, 0.0f}; }
    [[nodiscard]] static constexpr Vector2 One() noexcept { return {1.0f, 1.0f}; }
    [[nodiscard]] static constexpr Vector2 Up() noexcept { return {0.0f, -1.0f}; }
    [[nodiscard]] static constexpr Vector2 Down() noexcept { return {0.0f, 1.0f}; }
    [[nodiscard]] static constexpr Vector2 Left() noexcept { return {-1.0f, 0.0f}; }
    [[nodiscard]] static constexpr Vector2 Right() noexcept { return {1.0f, 0.0f}; }

    // Random vector generation
    [[nodiscard]] static Vector2 RandomUnit() noexcept;
    [[nodiscard]] static Vector2 RandomInRange(float minX, float maxX, float minY, float maxY) noexcept;

private:
    // Private helper for magnitude calculations with improved precision
    [[nodiscard]] static constexpr float SafeSqrt(float value) noexcept {
        return value > 0.0f ? std::sqrt(value) : 0.0f;
    }
};

// Free function operators for scalar * Vector2
[[nodiscard]] constexpr Vector2 operator*(float scalar, const Vector2& vec) noexcept {
    return vec * scalar;
}

// Stream output operator
std::ostream& operator<<(std::ostream& os, const Vector2& vec);

} // namespace RainEngine

// Type alias for backward compatibility
using Vector2 = RainEngine::Vector2;