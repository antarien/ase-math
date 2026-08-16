#pragma once

/**
 * ASE FOUNDATION HEADER
 *
 * @file        color3.hpp
 * @brief       Color3 - three-component linear RGB colour value
 * @description Component-wise arithmetic, clamping to the unit range, perceived luminance and
 *              linear interpolation. Header-only and stateless; clamping goes through
 *              ase::math::clamp rather than std::clamp.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @created     2025-12-17
 * @modified    2026-08-15
 * @version     1.0.0
 */

#include <ase/math/math.hpp>

namespace ase::math {

/**
 * RGB Color - Foundation for all color calculations
 */
struct Color3 {
    float r, g, b;

    constexpr Color3() : r(0), g(0), b(0) {}
    constexpr Color3(float r, float g, float b) : r(r), g(g), b(b) {}

    // Arithmetic operators
    constexpr Color3 operator+(const Color3& other) const {
        return {r + other.r, g + other.g, b + other.b};
    }

    constexpr Color3 operator-(const Color3& other) const {
        return {r - other.r, g - other.g, b - other.b};
    }

    constexpr Color3 operator*(float scalar) const {
        return {r * scalar, g * scalar, b * scalar};
    }

    constexpr Color3 operator*(const Color3& other) const {
        return {r * other.r, g * other.g, b * other.b};
    }

    constexpr Color3 operator/(float scalar) const {
        return {r / scalar, g / scalar, b / scalar};
    }

    // In-place operators
    Color3& operator+=(const Color3& other) {
        r += other.r; g += other.g; b += other.b;
        return *this;
    }

    Color3& operator-=(const Color3& other) {
        r -= other.r; g -= other.g; b -= other.b;
        return *this;
    }

    Color3& operator*=(float scalar) {
        r *= scalar; g *= scalar; b *= scalar;
        return *this;
    }

    // Clamp to [0, 1]
    Color3 clamped() const {
        return {
            ase::math::clamp(r, 0.0f, 1.0f),
            ase::math::clamp(g, 0.0f, 1.0f),
            ase::math::clamp(b, 0.0f, 1.0f)
        };
    }

    // Luminance (perceived brightness)
    constexpr float luminance() const {
        return 0.2126f * r + 0.7152f * g + 0.0722f * b;
    }

    // Linear interpolation
    static constexpr Color3 lerp(const Color3& a, const Color3& b, float t) {
        return {
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t
        };
    }

    // Static constructors
    static constexpr Color3 black() { return {0, 0, 0}; }
    static constexpr Color3 white() { return {1, 1, 1}; }
    static constexpr Color3 red() { return {1, 0, 0}; }
    static constexpr Color3 green() { return {0, 1, 0}; }
    static constexpr Color3 blue() { return {0, 0, 1}; }
    static constexpr Color3 yellow() { return {1, 1, 0}; }
    static constexpr Color3 cyan() { return {0, 1, 1}; }
    static constexpr Color3 magenta() { return {1, 0, 1}; }
};

// Free functions
inline Color3 operator*(float scalar, const Color3& c) {
    return c * scalar;
}

inline Color3 lerp(const Color3& a, const Color3& b, float t) {
    return Color3::lerp(a, b, t);
}

} // namespace ase::math
