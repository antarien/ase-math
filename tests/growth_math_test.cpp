/**
 * ASE MATH - Growth Geometry Unit Tests
 *
 * @file        growth_math_test.cpp
 * @brief       Cases for the three-dimensional noise, the Hermite segment, the chain simplification
 *              and the componentwise product.
 * @description The acceptance of step T.1 of PLAN_ASE_VIVARIUM_TREEGEN: the functions a growing
 *              plant reads to bend its axes (perlin3, fbm3_vec), to draw the curve between two
 *              nodes (hermite) and to thin an axis for its coarse levels (douglas_peucker,
 *              segment_distance_squared), plus mul.
 *
 *              perlin3: exactly 0 on every lattice point, within plus or minus NOISE3_BOUND
 *              everywhere, a mean near 0 over many cells, continuous across cell borders, and not
 *              constant. fbm3_vec: the zero vector without octaves, three channels that differ.
 *              hermite: both end points exact, both tangents met. douglas_peucker: a chain on one
 *              line keeps its two ends, a bent chain keeps its bend, the ends are always kept.
 *
 *              DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN is set by the build (tests/CMakeLists.txt,
 *              target_compile_definitions on the target ase-math-test).
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @usecase     usecase:101396 - assure engine component quality
 * @actor       Engine Unit Test (ENGUTEST)
 * @created     2026-11-19
 * @modified    2026-11-19
 * @version     00.00.26.00026 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>
#include <ase/math/geometry.hpp>
#include <ase/math/interpolation.hpp>
#include <ase/math/perlin.hpp>
#include <ase/math/vec3.hpp>

using namespace ase::math;

TEST_CASE("perlin3 - exactly zero on every lattice point") {
    for (int ix = -3; ix <= 3; ++ix) {
        for (int iy = -3; iy <= 3; ++iy) {
            for (int iz = -3; iz <= 3; ++iz) {
                const Vec3 p{static_cast<float>(ix), static_cast<float>(iy), static_cast<float>(iz)};
                CHECK(perlin3(p) == 0.0f);
            }
        }
    }
}

TEST_CASE("perlin3 - range, mean and variation over many cells") {
    const int steps = 40;
    const float step = 0.173f;
    float sum = 0.0f;
    float lowest = 0.0f;
    float highest = 0.0f;
    int samples = 0;
    for (int i = 0; i < steps; ++i) {
        for (int j = 0; j < steps; ++j) {
            for (int k = 0; k < steps; ++k) {
                const Vec3 p{i * step - 3.1f, j * step + 0.37f, k * step - 1.9f};
                const float v = perlin3(p);
                CHECK(v <= NOISE3_BOUND);
                CHECK(v >= -NOISE3_BOUND);
                sum += v;
                lowest = v < lowest ? v : lowest;
                highest = v > highest ? v : highest;
                ++samples;
            }
        }
    }
    const float mean = sum / static_cast<float>(samples);
    CHECK(mean < 0.05f);
    CHECK(mean > -0.05f);
    CHECK(highest > 0.2f);
    CHECK(lowest < -0.2f);
}

TEST_CASE("perlin3 - continuous across cell borders") {
    const float eps = 1.0e-3f;
    for (int i = 0; i < 64; ++i) {
        // Points just before and just after an integer border on each axis
        const float border = static_cast<float>(i % 8) - 4.0f;
        const Vec3 base{border - eps * 0.5f, 0.31f + 0.11f * static_cast<float>(i), 2.47f};
        const Vec3 across{border + eps * 0.5f, base.y, base.z};
        const float delta = perlin3(across) - perlin3(base);
        CHECK(delta < 0.01f);
        CHECK(delta > -0.01f);
    }
}

TEST_CASE("perlin3 - the same point yields the same value") {
    const Vec3 p{12.345f, -6.789f, 0.5f};
    CHECK(perlin3(p) == perlin3(p));
    CHECK(perlin3(p) != perlin3(p + Vec3{0.25f, 0.0f, 0.0f}));
}

TEST_CASE("fbm3_vec - zero without octaves, three distinct channels") {
    const Vec3 p{1.37f, -2.71f, 0.53f};

    const Vec3 none = fbm3_vec(p, 0, 0.5f, 2.0f);
    CHECK(none.x == 0.0f);
    CHECK(none.y == 0.0f);
    CHECK(none.z == 0.0f);

    const Vec3 one = fbm3_vec(p, 1, 0.5f, 2.0f);
    CHECK(one.x == perlin3(p));
    CHECK(one.y == perlin3(p + NOISE3_CHANNEL_OFFSET_Y));
    CHECK(one.z == perlin3(p + NOISE3_CHANNEL_OFFSET_Z));

    const Vec3 four = fbm3_vec(p, 4, 0.5f, 2.0f);
    CHECK(four.x != four.y);
    CHECK(four.y != four.z);
    CHECK(four.x <= NOISE3_BOUND);
    CHECK(four.x >= -NOISE3_BOUND);
}

TEST_CASE("hermite - end points exact, tangents met") {
    const Vec3 p0{0.0f, 0.0f, 0.0f};
    const Vec3 m0{0.0f, 0.0f, 2.0f};
    const Vec3 p1{0.5f, 0.25f, 1.5f};
    const Vec3 m1{1.0f, 0.0f, 1.0f};

    const Vec3 start = hermite(p0, m0, p1, m1, 0.0f);
    CHECK(start.x == p0.x);
    CHECK(start.y == p0.y);
    CHECK(start.z == p0.z);

    const Vec3 end = hermite(p0, m0, p1, m1, 1.0f);
    CHECK(end.x == p1.x);
    CHECK(end.y == p1.y);
    CHECK(end.z == p1.z);

    // One-sided difference quotients at both ends approach the tangents
    const float h = 1.0e-3f;
    const Vec3 d0 = (hermite(p0, m0, p1, m1, h) - start) / h;
    CHECK(d0.x == doctest::Approx(m0.x).epsilon(0.01));
    CHECK(d0.z == doctest::Approx(m0.z).epsilon(0.01));
    const Vec3 d1 = (end - hermite(p0, m0, p1, m1, 1.0f - h)) / h;
    CHECK(d1.x == doctest::Approx(m1.x).epsilon(0.01));
    CHECK(d1.z == doctest::Approx(m1.z).epsilon(0.01));
}

TEST_CASE("segment_distance_squared - interior, beyond the ends, degenerate") {
    const Vec3 a{0.0f, 0.0f, 0.0f};
    const Vec3 b{2.0f, 0.0f, 0.0f};
    CHECK(segment_distance_squared(Vec3{1.0f, 3.0f, 0.0f}, a, b) == 9.0f);
    CHECK(segment_distance_squared(Vec3{5.0f, 0.0f, 0.0f}, a, b) == 9.0f);
    CHECK(segment_distance_squared(Vec3{-1.0f, 0.0f, 0.0f}, a, b) == 1.0f);
    CHECK(segment_distance_squared(Vec3{0.0f, 4.0f, 0.0f}, a, a) == 16.0f);
}

TEST_CASE("douglas_peucker - straight, bent and degenerate chains") {
    SUBCASE("A chain on one line keeps exactly its two ends") {
        Vec3 chain[10];
        bool keep[10];
        for (int i = 0; i < 10; ++i) chain[i] = Vec3{0.0f, 0.0f, static_cast<float>(i) * 0.5f};
        CHECK(douglas_peucker(chain, 10, 0.01f, keep) == 2);
        CHECK(keep[0]);
        CHECK(keep[9]);
        for (int i = 1; i < 9; ++i) CHECK_FALSE(keep[i]);
    }

    SUBCASE("A bend beyond the tolerance stays, below it falls away") {
        Vec3 chain[5] = {
            Vec3{0.0f, 0.0f, 0.0f}, Vec3{0.0f, 0.0f, 1.0f}, Vec3{1.0f, 0.0f, 2.0f},
            Vec3{2.0f, 0.0f, 2.0f}, Vec3{3.0f, 0.0f, 2.0f}};
        bool keep[5];
        // Node 2 lies 1.11 from the chord 0-4 and stays; node 1 lies 0.45 from the chord 0-2 and
        // node 3 lies on the chord 2-4, so both fall below a tolerance of 0.5
        CHECK(douglas_peucker(chain, 5, 0.5f, keep) == 3);
        CHECK(keep[0]);
        CHECK(keep[2]);
        CHECK(keep[4]);
        CHECK(douglas_peucker(chain, 5, 10.0f, keep) == 2);
    }

    SUBCASE("Empty and single-node chains") {
        Vec3 chain[1] = {Vec3{1.0f, 2.0f, 3.0f}};
        bool keep[1] = {false};
        CHECK(douglas_peucker(chain, 0, 0.1f, keep) == 0);
        CHECK(douglas_peucker(chain, 1, 0.1f, keep) == 1);
        CHECK(keep[0]);
    }
}

TEST_CASE("mul - componentwise product") {
    const Vec3 r = mul(Vec3{1.0f, -2.0f, 3.0f}, Vec3{4.0f, 0.5f, -2.0f});
    CHECK(r.x == 4.0f);
    CHECK(r.y == -1.0f);
    CHECK(r.z == -6.0f);
}
