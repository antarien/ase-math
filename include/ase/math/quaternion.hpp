#pragma once

#include <ase/math/vec3.hpp>
#include <cmath>

namespace ase::math {

/**
 * Quaternion - Rotation representation without gimbal lock
 *
 * q = w + xi + yj + zk
 * where w is scalar part, (x,y,z) is vector part
 */
struct Quaternion {
    float w, x, y, z;

    constexpr Quaternion() : w(1), x(0), y(0), z(0) {}
    constexpr Quaternion(float w, float x, float y, float z) : w(w), x(x), y(y), z(z) {}

    // Identity quaternion (no rotation)
    static constexpr Quaternion identity() { return {1, 0, 0, 0}; }

    // Create from axis-angle
    static Quaternion from_axis_angle(const Vec3& axis, float angle) {
        const float half = angle * 0.5f;
        const float s = std::sin(half);
        const Vec3 n = axis.normalized();
        return {std::cos(half), n.x * s, n.y * s, n.z * s};
    }

    // Create from Euler angles (yaw, pitch, roll in radians)
    static Quaternion from_euler(float yaw, float pitch, float roll) {
        const float cy = std::cos(yaw * 0.5f);
        const float sy = std::sin(yaw * 0.5f);
        const float cp = std::cos(pitch * 0.5f);
        const float sp = std::sin(pitch * 0.5f);
        const float cr = std::cos(roll * 0.5f);
        const float sr = std::sin(roll * 0.5f);

        return {
            cr * cp * cy + sr * sp * sy,
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy
        };
    }

    // Quaternion multiplication (combines rotations)
    constexpr Quaternion operator*(const Quaternion& q) const {
        return {
            w * q.w - x * q.x - y * q.y - z * q.z,
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w
        };
    }

    // Rotate a vector by this quaternion
    Vec3 rotate(const Vec3& v) const {
        // q * v * q^-1 optimized
        const Vec3 qv{x, y, z};
        const Vec3 uv = cross(qv, v);
        const Vec3 uuv = cross(qv, uv);
        return v + ((uv * w) + uuv) * 2.0f;
    }

    // Conjugate (inverse for unit quaternions)
    constexpr Quaternion conjugate() const {
        return {w, -x, -y, -z};
    }

    // Magnitude
    float magnitude() const {
        return std::sqrt(w * w + x * x + y * y + z * z);
    }

    // Normalize to unit quaternion
    Quaternion normalized() const {
        const float mag = magnitude();
        return mag > 0 ? Quaternion{w / mag, x / mag, y / mag, z / mag} : identity();
    }

    // Inverse (for unit quaternions, same as conjugate)
    Quaternion inverse() const {
        return conjugate();
    }

    // Dot product (for slerp)
    constexpr float dot(const Quaternion& q) const {
        return w * q.w + x * q.x + y * q.y + z * q.z;
    }

    // Spherical linear interpolation
    static Quaternion slerp(const Quaternion& a, const Quaternion& b, float t) {
        float d = a.dot(b);

        Quaternion target = b;
        if (d < 0.0f) {
            d = -d;
            target = {-b.w, -b.x, -b.y, -b.z};
        }

        if (d > 0.9995f) {
            // Linear interpolation for very close quaternions
            return Quaternion{
                a.w + t * (target.w - a.w),
                a.x + t * (target.x - a.x),
                a.y + t * (target.y - a.y),
                a.z + t * (target.z - a.z)
            }.normalized();
        }

        const float theta = std::acos(d);
        const float sin_theta = std::sin(theta);
        const float wa = std::sin((1.0f - t) * theta) / sin_theta;
        const float wb = std::sin(t * theta) / sin_theta;

        return {
            wa * a.w + wb * target.w,
            wa * a.x + wb * target.x,
            wa * a.y + wb * target.y,
            wa * a.z + wb * target.z
        };
    }

    // Extract forward vector (local -Z rotated)
    Vec3 forward() const {
        return rotate(Vec3{0, 0, -1});
    }

    // Extract right vector (local +X rotated)
    Vec3 right() const {
        return rotate(Vec3{1, 0, 0});
    }

    // Extract up vector (local +Y rotated)
    Vec3 up() const {
        return rotate(Vec3{0, 1, 0});
    }

    // Get yaw angle (rotation around Y axis)
    float yaw() const {
        return std::atan2(2.0f * (w * y + x * z), 1.0f - 2.0f * (y * y + x * x));
    }

    // Get pitch angle (rotation around X axis)
    float pitch() const {
        const float sinp = 2.0f * (w * x - z * y);
        if (std::abs(sinp) >= 1.0f) {
            return std::copysign(3.14159265f / 2.0f, sinp);
        }
        return std::asin(sinp);
    }
};

// Free functions
inline Quaternion slerp(const Quaternion& a, const Quaternion& b, float t) {
    return Quaternion::slerp(a, b, t);
}

} // namespace ase::math
