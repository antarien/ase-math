#pragma once

/**
 * ASE Math Geometry Primitives
 *
 * @file        geometry.hpp
 * @brief       SSOT for geometric primitives (AABB, Sphere)
 * @description Layer 0 foundation types used by all modules.
 *              ase-spatial, ase-gis, ase-combat etc. import from here.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @created     2026-01-08
 * @modified    2026-08-15
 * @version     1.0.0
 */

#include <ase/math/vec3.hpp>
#include <ase/math/math.hpp>

namespace ase::math {

// =============================================================================
// AABB (Axis-Aligned Bounding Box)
// =============================================================================

struct AABB {
    Vec3 min{0, 0, 0};
    Vec3 max{0, 0, 0};

    AABB() = default;
    AABB(Vec3 min_, Vec3 max_) : min(min_), max(max_) {}

    [[nodiscard]] Vec3 center() const {
        return (min + max) * 0.5f;
    }

    [[nodiscard]] Vec3 size() const {
        return max - min;
    }

    [[nodiscard]] bool contains(const Vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }

    [[nodiscard]] bool intersects(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }

    static AABB from_center_radius(const Vec3& center, float radius) {
        return AABB{
            Vec3{center.x - radius, center.y - radius, center.z - radius},
            Vec3{center.x + radius, center.y + radius, center.z + radius}
        };
    }
};

// =============================================================================
// SPHERE
// =============================================================================

struct Sphere {
    Vec3 center{0, 0, 0};
    float radius = 0.0f;

    Sphere() = default;
    Sphere(Vec3 c, float r) : center(c), radius(r) {}

    [[nodiscard]] bool contains(const Vec3& point) const {
        Vec3 diff = point - center;
        return diff.length_squared() <= (radius * radius);
    }

    [[nodiscard]] bool intersects(const AABB& box) const {
        // Find closest point on AABB to sphere center
        float x = clamp(center.x, box.min.x, box.max.x);
        float y = clamp(center.y, box.min.y, box.max.y);
        float z = clamp(center.z, box.min.z, box.max.z);

        // Check if closest point is within sphere
        float dx = x - center.x;
        float dy = y - center.y;
        float dz = z - center.z;

        return (dx*dx + dy*dy + dz*dz) <= (radius * radius);
    }

    [[nodiscard]] bool intersects(const Sphere& other) const {
        Vec3 diff = other.center - center;
        float combined_radius = radius + other.radius;
        return diff.length_squared() <= (combined_radius * combined_radius);
    }
};

// =============================================================================
// FREE FUNCTIONS (for consistent interface)
// =============================================================================

/**
 * Check if point is inside sphere.
 */
inline bool point_in_sphere(const Vec3& point, const Vec3& center, float radius) {
    Vec3 diff = point - center;
    return diff.length_squared() <= (radius * radius);
}

/**
 * Check if point is inside AABB.
 */
inline bool point_in_aabb(const Vec3& point, const Vec3& min, const Vec3& max) {
    return point.x >= min.x && point.x <= max.x &&
           point.y >= min.y && point.y <= max.y &&
           point.z >= min.z && point.z <= max.z;
}

/**
 * Squared distance between two points (avoids sqrt).
 */
inline float distance_squared(const Vec3& a, const Vec3& b) {
    Vec3 diff = b - a;
    return diff.length_squared();
}

/**
 * Squared distance between two points (6 float overload).
 */
inline float distance_squared(float x1, float y1, float z1,
                               float x2, float y2, float z2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return dx*dx + dy*dy + dz*dz;
}

/**
 * Distance between two points.
 */
inline float distance(const Vec3& a, const Vec3& b) {
    return sqrt(distance_squared(a, b));
}

}  // namespace ase::math
