#pragma once

/**
 * ASE FOUNDATION HEADER
 *
 * @file        interpolation.hpp
 * @brief       Interpolation and easing helpers shared by every module
 * @description Linear and smooth interpolation, inverse lerp and remapping between ranges.
 *              Header-only and stateless; the single source for interpolation so consumer
 *              systems carry no second implementation.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @created     2026-01-08
 * @modified    2026-08-15
 * @version     1.0.0
 */

#include <algorithm>
#include <cmath>

namespace ase::math {

/**
 * Linear interpolation between two values.
 */
constexpr float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

/**
 * Clamp value to [0, 1] range.
 */
constexpr float saturate(float t) {
    return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
}

/**
 * Hermite smoothstep interpolation.
 * Returns smooth curve from 0 to 1 as t goes from 0 to 1.
 */
constexpr float smoothstep(float t) {
    t = saturate(t);
    return t * t * (3.0f - 2.0f * t);
}

/**
 * Ken Perlin's improved smoothstep (smootherstep).
 * Smoother curve with zero first and second derivatives at endpoints.
 */
constexpr float smootherstep(float t) {
    t = saturate(t);
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

/**
 * Remap value from one range to another.
 */
constexpr float remap(float value, float in_min, float in_max, float out_min, float out_max) {
    float t = (value - in_min) / (in_max - in_min);
    return out_min + t * (out_max - out_min);
}

}  // namespace ase::math
