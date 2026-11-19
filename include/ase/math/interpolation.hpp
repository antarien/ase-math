#pragma once

/**
 * ASE FOUNDATION HEADER
 *
 * @file        interpolation.hpp
 * @brief       Interpolation and easing helpers shared by every module
 * @description Linear and smooth interpolation, inverse lerp and remapping between ranges, and
 *              the cubic Hermite segment between two points with their tangents (the curve of a
 *              growing axis between two nodes). Header-only and stateless; the single source for
 *              interpolation so consumer systems carry no second implementation.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @usecase     usecase:101541 - compute scalar and vector math
 * @actor       Math Library Steward (MATHLIBSTE)
 * @created     2026-01-08
 * @modified    2026-11-19
 * @version     00.00.26.00026 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/math/vec3.hpp>

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

/**
 * The two factors of the cubic Hermite basis. They are the coefficients of the four basis
 * polynomials h00 = 2t^3 - 3t^2 + 1, h10 = t^3 - 2t^2 + t, h01 = -2t^3 + 3t^2, h11 = t^3 - t^2.
 */
constexpr float HERMITE_CUBIC_FACTOR = 2.0f;     // weight of t^3 in h00 and h01, of t^2 in h10
constexpr float HERMITE_QUADRATIC_FACTOR = 3.0f; // weight of t^2 in h00 and h01

/**
 * @brief Point on the cubic Hermite segment from p0 to p1
 * @param p0  start point, reached exactly at t = 0
 * @param m0  tangent at the start point (one segment length per unit of t)
 * @param p1  end point, reached exactly at t = 1
 * @param m1  tangent at the end point
 * @param t   curve parameter; [0, 1] lies on the segment, outside it extrapolates the cubic
 * @return the point; the derivative at t = 0 is m0 and at t = 1 is m1
 *
 * Two neighbouring segments that share a node and its tangent join without a kink, which is
 * what a branch axis built node by node needs: the growth step sets the node, the coherent
 * deviation sets the tangent, and the curve between them follows from both alone.
 */
inline Vec3 hermite(const Vec3& p0, const Vec3& m0, const Vec3& p1, const Vec3& m1, float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float h00 = HERMITE_CUBIC_FACTOR * t3 - HERMITE_QUADRATIC_FACTOR * t2 + 1.0f;
    const float h10 = t3 - HERMITE_CUBIC_FACTOR * t2 + t;
    const float h01 = HERMITE_QUADRATIC_FACTOR * t2 - HERMITE_CUBIC_FACTOR * t3;
    const float h11 = t3 - t2;
    return p0 * h00 + m0 * h10 + p1 * h01 + m1 * h11;
}

}  // namespace ase::math
