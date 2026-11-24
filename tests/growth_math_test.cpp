/**
 * ASE MATH - Growth Geometry Unit Tests
 *
 * @file        growth_math_test.cpp
 * @brief       Cases for the three-dimensional noise, the Hermite segment and the componentwise
 *              product.
 * @description The acceptance of step T.1 of PLAN_ASE_VIVARIUM_TREEGEN: the functions a growing
 *              plant reads to bend its axes (perlin3, fbm3_vec) and to draw the curve between two
 *              nodes (hermite), plus mul. The knot selection per detail level is graphics; its
 *              owner ase-render-data tests it in tests/test_render_data_cyl.cpp (register P0714).
 *
 *              perlin3: exactly 0 on every lattice point, within plus or minus NOISE3_BOUND
 *              everywhere, a mean near 0 over many cells, continuous across cell borders, and not
 *              constant, and at six fixed points the float32 value of the step-by-step reference, so
 *              Linux and Android show the same bits. fbm3_vec: the zero vector without octaves, three
 *              channels that differ.
 *              hermite: both end points exact, both tangents met.
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
 * @modified    2026-11-23
 * @version     00.00.26.00027 [feat]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>
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

TEST_CASE("perlin3 - the same bits on every device") {
    // The references are IEEE-754 single precision, computed step by step in the order of perlin.hpp
    // (data/probes/t1_noise_bits_1123.py, run data/probes/runs/t1_noise_bits_1123.txt). Both builds compile with
    // -ffp-contract=off, so no multiply-add is fused and every step rounds alone; exact equality to the hex literal
    // on Linux and on Android is the same bit pattern on both. The points are dyadic, so the literal of the point is
    // the float32 the reference used, and a negative coordinate covers floor and the two's complement corner hash.
    CHECK(detail::noise3_corner(0, 0, 0) == 0x435416F0u);
    CHECK(detail::noise3_corner(-1, 2, -3) == 0x8BAE6F79u);
    CHECK(perlin3(Vec3{0.375f, 0.625f, 0.125f}) == 0x1.45600cp-2f);
    CHECK(perlin3(Vec3{3.71875f, -2.40625f, 0.5625f}) == 0x1.0cccp-3f);
    CHECK(perlin3(Vec3{-7.15625f, 11.84375f, -3.03125f}) == 0x1.28e7d8p-2f);
    CHECK(perlin3(Vec3{12.34375f, -6.78125f, 0.5f}) == 0x1.abc4cp-4f);
    CHECK(perlin3(Vec3{100.8125f, 42.1875f, -55.4375f}) == 0x1.ef5f74p-3f);
    CHECK(perlin3(Vec3{-0.96875f, -0.03125f, 0.90625f}) == 0x1.f7f73cp-5f);
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

TEST_CASE("mul - componentwise product") {
    const Vec3 r = mul(Vec3{1.0f, -2.0f, 3.0f}, Vec3{4.0f, 0.5f, -2.0f});
    CHECK(r.x == 4.0f);
    CHECK(r.y == -1.0f);
    CHECK(r.z == -6.0f);
}
