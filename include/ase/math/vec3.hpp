#pragma once

/**
 * ASE FOUNDATION HEADER
 *
 * @file        vec3.hpp
 * @brief       Vec3 - three-component float vector, the spatial primitive of the engine
 * @description Arithmetic, dot and cross product, length and normalisation, including the
 *              XY-plane variants for the world's horizontal plane (the height axis is Z).
 *              Header-only and stateless; square roots go through ase::math::sqrt.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @created     2025-12-15
 * @modified    2026-08-15
 * @version     1.0.0
 */

#include <ase/math/math.hpp>

namespace ase::math {

/**
 * 3D Vector - Foundation for all spatial calculations
 */
struct Vec3 {
    float x, y, z;

    constexpr Vec3() : x(0), y(0), z(0) {}
    constexpr Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Arithmetic operators
    constexpr Vec3 operator+(const Vec3& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }

    constexpr Vec3 operator-(const Vec3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    constexpr Vec3 operator*(float scalar) const {
        return {x * scalar, y * scalar, z * scalar};
    }

    constexpr Vec3 operator/(float scalar) const {
        return {x / scalar, y / scalar, z / scalar};
    }

    // In-place operators
    Vec3& operator+=(const Vec3& other) {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }

    Vec3& operator-=(const Vec3& other) {
        x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }

    Vec3& operator*=(float scalar) {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }

    // Dot product
    constexpr float dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    // Cross product
    constexpr Vec3 cross(const Vec3& other) const {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }

    // Length
    float length() const {
        return ase::math::sqrt(x * x + y * y + z * z);
    }

    float length_squared() const {
        return x * x + y * y + z * z;
    }

    /* HORIZONTAL MEANS X AND Y IN THIS ENGINE, AND THE PAIR BELOW USED TO SAY XZ (2026-08-15).
     *
     * The height axis of ASE is Z: hub_metrics.json states it for every position family it
     * declares ("SPT_POS_Y: HORIZONTAL AXIS, the second of two - never the height",
     * "SPT_POS_Z: THE HEIGHT AXIS"). A helper that calls sqrt(x*x + z*z) "horizontal distance"
     * therefore mixes ONE horizontal with the HEIGHT and silently drops the other horizontal - and
     * the normalising twin zeroed y, so a direction taken from it could never point along the
     * second horizontal at all, while it could point straight up.
     *
     * The old pair belonged to a y-up world. ase-math still HAS one, deliberately, in
     * spherical.hpp ("Y is UP") - that frame is the planet's own, where y is the pole, and
     * modules/ase-geoid plus plugins/ase-pl-sky use it correctly for icosahedron and star dome.
     * What must not happen is to reach for a y-up helper while computing in the WORLD plane. The
     * two frames are both legitimate; only their mixture is a defect.
     *
     * Measured before the swap: length_xz had exactly ONE caller in the whole tree,
     * normalized_xz one, and length_xz_squared none at all - all in
     * modules/ase-signature/src/tracking/signature_trk_sys.cpp, and all three were the mixture.
     */

    // Length in the XY plane - the horizontal plane of the world, height is Z
    float length_xy() const {
        return ase::math::sqrt(x * x + y * y);
    }

    float length_xy_squared() const {
        return x * x + y * y;
    }

    // Normalize
    Vec3 normalized() const {
        const float len = length();
        return len > 0 ? (*this / len) : Vec3{};
    }

    // Normalize in the XY plane only (z, the height, becomes 0)
    Vec3 normalized_xy() const {
        const float len = length_xy();
        return len > 0 ? Vec3{x / len, y / len, 0} : Vec3{};
    }

    // Distance
    float distance_to(const Vec3& other) const {
        return (*this - other).length();
    }

    // Static constructors
    static constexpr Vec3 zero() { return {0, 0, 0}; }
    static constexpr Vec3 one() { return {1, 1, 1}; }
    static constexpr Vec3 up() { return {0, 1, 0}; }
    static constexpr Vec3 forward() { return {0, 0, 1}; }
    static constexpr Vec3 right() { return {1, 0, 0}; }
};

// Free functions
inline Vec3 operator*(float scalar, const Vec3& v) {
    return v * scalar;
}

// Component factory. The operators above are members, so an expression like a + b has no
// counterpart in a language without operator overloading; code that has to survive the C++ to
// TypeScript transpiler spells the same arithmetic as vec3(a.x + b.x, a.y + b.y, a.z + b.z).
inline Vec3 vec3(float x, float y, float z) {
    return Vec3{x, y, z};
}

inline float dot(const Vec3& a, const Vec3& b) {
    return a.dot(b);
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return a.cross(b);
}

inline Vec3 normalize(const Vec3& v) {
    return v.normalized();
}

} // namespace ase::math
