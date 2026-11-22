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
 * @usecase     usecase:101541 - compute scalar and vector math
 * @actor       Math Library Steward (MATHLIBSTE)
 * @created     2026-01-08
 * @modified    2026-10-30
 * @version     00.00.26.00026 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/math/vec3.hpp>
#include <ase/math/math.hpp>

namespace ase::math {

// AABB (axis-aligned bounding box)

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

// Sphere

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

// Free functions (for a consistent interface)

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

/**
 * Closest point of the triangle (a, b, c) to the point p.
 *
 * The point of the triangle - its face, one of its three edges or one of its three corners - that lies nearest
 * to p, after the Voronoi regions of the triangle (Ericson, Real-Time Collision Detection, 5.1.5). A sphere
 * collides with the triangle when this point lies closer to its centre than its radius; the line from the point
 * to the centre is the normal the sphere is pushed out along.
 */
inline Vec3 closest_point_on_triangle(const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c) {
    const Vec3 ab = b - a;
    const Vec3 ac = c - a;
    const Vec3 ap = p - a;
    const float d1 = dot(ab, ap);
    const float d2 = dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a;

    const Vec3 bp = p - b;
    const float d3 = dot(ab, bp);
    const float d4 = dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b;

    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) return a + ab * (d1 / (d1 - d3));

    const Vec3 cp = p - c;
    const float d5 = dot(ab, cp);
    const float d6 = dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c;

    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) return a + ac * (d2 / (d2 - d6));

    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
    }

    const float denom = 1.0f / (va + vb + vc);
    return a + ab * (vb * denom) + ac * (vc * denom);
}

}  // namespace ase::math
