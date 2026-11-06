#pragma once

/**
 * ASE FOUNDATION HEADER
 *
 * @file        scalar.hpp
 * @brief       The engine's own scalar arithmetic - angle constants and generic operations
 * @description Angle constants and the small generic operations every tier uses: abs, min,
 *              max, clamp, lerp, sign. These are DEFINED here and delegate to nothing; they
 *              depend on no standard-library header at all. The header which merely FORWARDS
 *              to <cmath> (math.hpp) and the header which holds the engine's own arithmetic
 *              (this one) are two different files.
 *
 *              The cut runs in this direction on purpose. math.hpp is the wrapper, because
 *              five std:: bans exempt that exact path by name - with the recorded reason
 *              "math.hpp IS ase::math::sqrt, its body must call std::sqrt". Moving the
 *              wrapper bodies to another file would carry them out of an exemption the tree
 *              granted on purpose, so the native half stands here.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @usecase     usecase:101541 - compute scalar and vector math
 * @actor       Math Library Steward (MATHLIBSTE)
 * @created     2026-08-15
 * @modified    2026-10-30
 * @version     1.0.1
 */

namespace ase::math {

// Constants

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI / 2.0f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// Basic math functions - defined here, delegated nowhere

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
