#pragma once

#include <cmath>
#include <array>

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
        return std::sqrt(x * x + y * y + z * z);
    }

    float length_squared() const {
        return x * x + y * y + z * z;
    }

    // Length in XZ plane (horizontal distance)
    float length_xz() const {
        return std::sqrt(x * x + z * z);
    }

    float length_xz_squared() const {
        return x * x + z * z;
    }

    // Normalize
    Vec3 normalized() const {
        const float len = length();
        return len > 0 ? (*this / len) : Vec3{};
    }

    // Normalize in XZ plane only (y becomes 0)
    Vec3 normalized_xz() const {
        const float len = length_xz();
        return len > 0 ? Vec3{x / len, 0, z / len} : Vec3{};
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
