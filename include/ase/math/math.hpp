#pragma once

#include <cmath>
#include <algorithm>

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
// BASIC MATH FUNCTIONS
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

// =============================================================================
// FLOATING POINT FUNCTIONS (use std:: for these)
// =============================================================================

inline float sqrt(float value) {
    return std::sqrt(value);
}

inline float sin(float value) {
    return std::sin(value);
}

inline float cos(float value) {
    return std::cos(value);
}

inline float tan(float value) {
    return std::tan(value);
}

inline float asin(float value) {
    return std::asin(value);
}

inline float acos(float value) {
    return std::acos(value);
}

inline float atan2(float y, float x) {
    return std::atan2(y, x);
}

inline float fmod(float x, float y) {
    return std::fmod(x, y);
}

inline float floor(float value) {
    return std::floor(value);
}

inline float ceil(float value) {
    return std::ceil(value);
}

inline float round(float value) {
    return std::round(value);
}

inline float pow(float base, float exp) {
    return std::pow(base, exp);
}

inline float exp(float value) {
    return std::exp(value);
}

inline float log(float value) {
    return std::log(value);
}

}  // namespace ase::math
