#include "Vector2.h"
#include "RandomGenerator.h"
#include <iostream>
#ifdef __cpp_lib_math_constants
    #include <numbers>
#endif

namespace RainEngine {

Vector2 Vector2::RandomUnit() noexcept {
    auto& rng = RandomGenerator::GetInstance();
    #ifdef __cpp_lib_math_constants
        const auto angle = rng.GenerateFloat(0.0f, 2.0f * std::numbers::pi_v<float>);
    #else
        constexpr float PI = 3.14159265359f;
        const auto angle = rng.GenerateFloat(0.0f, 2.0f * PI);
    #endif
    return {std::cos(angle), std::sin(angle)};
}

Vector2 Vector2::RandomInRange(float minX, float maxX, float minY, float maxY) noexcept {
    auto& rng = RandomGenerator::GetInstance();
    return {
        rng.GenerateFloat(minX, maxX),
        rng.GenerateFloat(minY, maxY)
    };
}

std::ostream& operator<<(std::ostream& os, const Vector2& vec) {
    return os << vec.ToString();
}

} // namespace RainEngine