#pragma once

/**
 * ASE Math Polygon Geometry
 *
 * @file        polygon.hpp
 * @brief       SSOT for 2D polygon geometry operations
 * @description Layer 0 foundation functions used by ase-gis, ase-spatial, etc.
 *              All polygon-related math belongs HERE, not in consumer systems!
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @created     2026-01-08
 * @modified    2026-01-08
 * @version     1.0.0
 *
 * DRY / SOLID / SSOT COMPLIANCE:
 * - point_in_polygon() - ONE implementation here
 * - polygon_bbox() - ONE implementation here
 * - distance_to_polygon_edge() - ONE implementation here
 * - NO duplicates in consumer modules!
 */

#include <ase/math/math.hpp>
#include <cstdint>
#include <cstddef>

namespace ase::math {

/**
 * @brief Point-in-polygon test using ray casting algorithm
 * @param px Point X coordinate
 * @param py Point Y coordinate
 * @param vertices Array of vertex coordinates [x0,y0,x1,y1,...]
 * @param vertex_count Number of vertices (NOT array length!)
 * @return true if point is inside polygon
 */
inline bool point_in_polygon(float px, float py, const float* vertices, size_t vertex_count) {
    if (vertex_count < 3 || vertices == nullptr) return false;

    bool inside = false;
    size_t j = vertex_count - 1;

    for (size_t i = 0; i < vertex_count; ++i) {
        float xi = vertices[i * 2];
        float yi = vertices[i * 2 + 1];
        float xj = vertices[j * 2];
        float yj = vertices[j * 2 + 1];

        if (((yi > py) != (yj > py)) &&
            (px < (xj - xi) * (py - yi) / (yj - yi) + xi)) {
            inside = !inside;
        }
        j = i;
    }

    return inside;
}

/**
 * @brief Calculate polygon bounding box
 * @param vertices Array of vertex coordinates [x0,y0,x1,y1,...]
 * @param vertex_count Number of vertices
 * @param[out] min_x Minimum X
 * @param[out] min_y Minimum Y
 * @param[out] max_x Maximum X
 * @param[out] max_y Maximum Y
 */
inline void polygon_bbox(const float* vertices, size_t vertex_count,
                         float& min_x, float& min_y, float& max_x, float& max_y) {
    if (vertex_count == 0 || vertices == nullptr) {
        min_x = min_y = max_x = max_y = 0.0f;
        return;
    }

    min_x = max_x = vertices[0];
    min_y = max_y = vertices[1];

    for (size_t i = 1; i < vertex_count; ++i) {
        float x = vertices[i * 2];
        float y = vertices[i * 2 + 1];
        if (x < min_x) min_x = x;
        if (y < min_y) min_y = y;
        if (x > max_x) max_x = x;
        if (y > max_y) max_y = y;
    }
}

/**
 * @brief Distance from point to nearest polygon edge
 * @param px Point X coordinate
 * @param py Point Y coordinate
 * @param vertices Array of vertex coordinates [x0,y0,x1,y1,...]
 * @param vertex_count Number of vertices
 * @return Distance to nearest edge (0 if on edge)
 */
inline float distance_to_polygon_edge(float px, float py, const float* vertices, size_t vertex_count) {
    if (vertex_count < 3 || vertices == nullptr) return 0.0f;

    float min_dist = 1e10f;
    size_t j = vertex_count - 1;

    for (size_t i = 0; i < vertex_count; ++i) {
        float x1 = vertices[j * 2];
        float y1 = vertices[j * 2 + 1];
        float x2 = vertices[i * 2];
        float y2 = vertices[i * 2 + 1];

        // Distance from point to line segment
        float dx = x2 - x1;
        float dy = y2 - y1;
        float len_sq = dx * dx + dy * dy;

        float t = 0.0f;
        if (len_sq > 0.0001f) {
            t = clamp(((px - x1) * dx + (py - y1) * dy) / len_sq, 0.0f, 1.0f);
        }

        float closest_x = x1 + t * dx;
        float closest_y = y1 + t * dy;
        float dist = sqrt((px - closest_x) * (px - closest_x) +
                          (py - closest_y) * (py - closest_y));

        if (dist < min_dist) min_dist = dist;
        j = i;
    }

    return min_dist;
}

/**
 * @brief Distance from point to line segment
 * @param px Point X
 * @param py Point Y
 * @param x1 Line start X
 * @param y1 Line start Y
 * @param x2 Line end X
 * @param y2 Line end Y
 * @return Distance to line segment
 */
inline float distance_to_line_segment(float px, float py,
                                       float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len_sq = dx * dx + dy * dy;

    float t = 0.0f;
    if (len_sq > 0.0001f) {
        t = clamp(((px - x1) * dx + (py - y1) * dy) / len_sq, 0.0f, 1.0f);
    }

    float closest_x = x1 + t * dx;
    float closest_y = y1 + t * dy;
    return sqrt((px - closest_x) * (px - closest_x) +
                (py - closest_y) * (py - closest_y));
}

}  // namespace ase::math
