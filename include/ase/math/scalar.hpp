#pragma once

/**
 * ASE FOUNDATION HEADER
 *
 * @file        scalar.hpp
 * @brief       The engine's own scalar arithmetic - angle constants and generic operations
 * @description Angle constants and the small generic operations every tier uses: abs, min,
 *              max, clamp, lerp, sign. These are DEFINED here and delegate to nothing; they
 *              depend on no standard-library header at all. They were split out of math.hpp
 *              on 2026-08-15 so that the header which merely FORWARDS to <cmath> and the
 *              header which holds the engine's own arithmetic are two different files.
 *
 *              The split is deliberately in this direction. math.hpp stays the wrapper,
 *              because five std:: bans exempt that exact path by name — with the recorded
 *              reason "math.hpp IS ase::math::sqrt, its body must call std::sqrt". Moving the
 *              wrapper bodies to a new file would have carried them out of an exemption the
 *              tree granted on purpose, so the native half moved instead.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @created     2026-08-15
 * @modified    2026-08-15
 * @version     1.0.0
 */

namespace ase::math {

// =============================================================================
// CONSTANTS
// =============================================================================

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI / 2.0f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// =============================================================================
// BASIC MATH FUNCTIONS - defined here, delegated nowhere
// =============================================================================

template<typename T>
constexpr T abs(T value) {
    return value < T(0) ? -value : value;
}

template<typename T>
constexpr T min(T a, T b) {
    return a < b ? a : b;
}

template<typename T>
constexpr T max(T a, T b) {
    return a > b ? a : b;
}

template<typename T>
constexpr T clamp(T value, T min_val, T max_val) {
    return min(max(value, min_val), max_val);
}

template<typename T>
constexpr T lerp(T a, T b, float t) {
    return a + (b - a) * t;
}

template<typename T>
constexpr int sign(T value) {
    return (T(0) < value) - (value < T(0));
}

}  // namespace ase::math
