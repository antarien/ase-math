#pragma once

/**
 * ASE FOUNDATION HEADER
 *
 * @file        perlin.hpp
 * @brief       Perlin - deterministic gradient noise in two and three dimensions, with fBm layering
 * @description Ken Perlin's improved noise. The class Perlin is the two-dimensional field of the
 *              terrain, seeded through a small LCG so the same seed always yields the same field;
 *              it holds a 512-entry permutation table as a plain C array.
 *
 *              perlin3 and fbm3_vec are the three-dimensional field of plant growth (the coherent
 *              deviation of an axis, the bark noise). Their lattice gradient is chosen by
 *              random_hash_u32 of the corner's integer coordinates instead of a permutation table,
 *              so the field holds no state and needs no seed object: the caller moves its sample
 *              point (an axis adds an offset drawn from its seed and lineage).
 *
 *              BIT-EQUAL ON LINUX AND ANDROID. perlin3 is integer hashing, floor, addition and
 *              multiplication and nothing else - no sin, cos or other libm call, whose last bit
 *              differs between glibc and bionic. Both builds compile with -ffp-contract=off
 *              (cmake/CompilerFlags.cmake, clients/avt-client-android/CMakeLists.txt), so no
 *              compiler fuses a product and a sum into one rounding. The coherence of a growing
 *              plant is simulation, and the same seed must grow the same tree on server and
 *              handset (PLAN_ASE_VIVARIUM_TREEGEN, "Kohaerente Wuchsabweichung").
 *
 *              RANGE OF perlin3. A lattice point yields exactly 0. Between points the value stays
 *              within plus or minus NOISE3_BOUND, the bound of a three-dimensional gradient field
 *              whose gradients have length sqrt(2). The integer cell of each coordinate must fit
 *              into int32_t; world positions in metres lie far inside that.
 *
 *              floor goes through ase::math::floor rather than std::floor.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @usecase     usecase:101541 - compute scalar and vector math
 * @actor       Math Library Steward (MATHLIBSTE)
 * @created     2025-12-16
 * @modified    2026-11-19
 * @version     00.00.26.00026 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/math/interpolation.hpp>
#include <ase/math/math.hpp>
#include <ase/math/random.hpp>
#include <ase/math/vec3.hpp>

#include <cstdint>

namespace ase::math {

class Perlin {
public:
    /**
     * Create Perlin noise generator with seed
     */
    explicit Perlin(uint32_t seed = 0) {
        // Initialize permutation table
        for (uint32_t i = 0u; i < 256u; ++i) {
            perm_[i] = static_cast<uint8_t>(i);
        }

        // Shuffle using seed. The swap is spelled out rather than taken from <utility>:
        // this header pulls in nothing beyond <cstdint> and the engine's own math.
        uint32_t s = seed;
        for (uint32_t i = 255u; i > 0u; --i) {
            s = lcg(s);
            const uint32_t j = s % (i + 1u);
            const uint8_t tmp = perm_[i];
            perm_[i] = perm_[j];
            perm_[j] = tmp;
        }

        // Duplicate for overflow
        for (uint32_t i = 0u; i < 256u; ++i) {
            perm_[256u + i] = perm_[i];
        }
    }

    /**
     * 2D Perlin noise at position (x, y)
     * Returns value in range [-1, 1]
     */
    float noise(float x, float y) const {
        // Find unit grid cell containing point
        const int xi = static_cast<int>(ase::math::floor(x)) & 255;
        const int yi = static_cast<int>(ase::math::floor(y)) & 255;

        // Relative position within cell
        const float xf = x - ase::math::floor(x);
        const float yf = y - ase::math::floor(y);

        // Fade curves
        const float fade_x = fade(xf);
        const float fade_y = fade(yf);

        // Hash coordinates of 4 corners
        const int aa = perm_[perm_[xi] + yi];
        const int ab = perm_[perm_[xi] + yi + 1];
        const int ba = perm_[perm_[xi + 1] + yi];
        const int bb = perm_[perm_[xi + 1] + yi + 1];

        // Gradient values at corners
        const float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1, yf), fade_x);
        const float x2 = lerp(grad(ab, xf, yf - 1), grad(bb, xf - 1, yf - 1), fade_x);

        return lerp(x1, x2, fade_y);
    }

    /**
     * Fractal Brownian Motion (fBm) - layered noise
     * @param x, y    Position
     * @param octaves Number of noise layers (4-8 typical)
     * @param persistence Amplitude decay per octave (0.5 typical)
     * @param lacunarity Frequency increase per octave (2.0 typical)
     */
    float fbm(float x, float y, int octaves = 4, float persistence = 0.5f, float lacunarity = 2.0f) const {
        float total = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float max_value = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            total += noise(x * frequency, y * frequency) * amplitude;
            max_value += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }

        return total / max_value;  // Normalize to [-1, 1]
    }

private:
    // 256 entries duplicated once, so a hashed index plus its neighbour never runs past the end.
    uint8_t perm_[512] = {};

    // Linear congruential generator for seeding
    static uint32_t lcg(uint32_t x) {
        return x * 1103515245u + 12345u;
    }

    // Fade function: 6t^5 - 15t^4 + 10t^3
    static float fade(float t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    // Linear interpolation
    static float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    // Gradient function (2D)
    static float grad(int hash, float x, float y) {
        const int h = hash & 7;
        const float gu = h < 4 ? x : y;
        const float gv = h < 4 ? y : x;
        return ((h & 1) ? -gu : gu) + ((h & 2) ? -2.0f * gv : 2.0f * gv);
    }
};

/**
 * The constants of the three-dimensional field. They are part of the contract like the
 * RANDOM_HASH_* constants of random.hpp: a changed value is a changed shape of every plant whose
 * growth reads the field.
 */
constexpr uint32_t NOISE3_LATTICE_SEED = 0x2545F491u;  // seed of the lattice hash, fixed: the caller moves the point, never the lattice
constexpr uint32_t NOISE3_GRADIENT_MASK = 15u;         // four bits of the corner hash pick one of Perlin's sixteen gradient cases
constexpr uint32_t NOISE3_GRADIENT_SPLIT_U = 8u;       // cases 0..7 take x as first axis, 8..15 take y
constexpr uint32_t NOISE3_GRADIENT_SPLIT_V = 4u;       // cases 0..3 take y as second axis
constexpr uint32_t NOISE3_GRADIENT_CASE_X_A = 12u;     // case 12 takes x as second axis (repeats an edge, keeps sixteen cases)
constexpr uint32_t NOISE3_GRADIENT_CASE_X_B = 14u;     // case 14 takes x as second axis (repeats an edge, keeps sixteen cases)
constexpr uint32_t NOISE3_GRADIENT_SIGN_U = 1u;        // bit that negates the first axis
constexpr uint32_t NOISE3_GRADIENT_SIGN_V = 2u;        // bit that negates the second axis
constexpr float NOISE3_BOUND = 1.22474487f;            // sqrt(3/2): bound of a 3D gradient field with gradient length sqrt(2)
constexpr Vec3 NOISE3_CHANNEL_OFFSET_Y{31.416f, 47.853f, 12.718f};  // moves the second channel of fbm3_vec off the first, no lattice multiple
constexpr Vec3 NOISE3_CHANNEL_OFFSET_Z{73.205f, 19.937f, 58.661f};  // moves the third channel off the first two, no lattice multiple

namespace detail {

/**
 * @brief The 32-bit hash of one lattice corner
 * @param ix, iy, iz  integer cell coordinates; the cast to uint32_t wraps by definition
 * @return the mixed word; its low four bits pick the gradient
 */
[[nodiscard]] inline uint32_t noise3_corner(int32_t ix, int32_t iy, int32_t iz) noexcept {
    const uint32_t hx = random_hash_u32(NOISE3_LATTICE_SEED, static_cast<uint32_t>(ix));
    const uint32_t hy = random_hash_u32(hx, static_cast<uint32_t>(iy));
    return random_hash_u32(hy, static_cast<uint32_t>(iz));
}

/**
 * @brief Dot product of a corner gradient with the offset from that corner
 *
 * Perlin's reference choice (Improved Noise, 2002): the twelve edge directions of the cube, four
 * of them twice, so that four bits select a case without a modulo.
 */
[[nodiscard]] inline float noise3_grad(uint32_t hash, float x, float y, float z) noexcept {
    const uint32_t h = hash & NOISE3_GRADIENT_MASK;
    const float u = h < NOISE3_GRADIENT_SPLIT_U ? x : y;
    const bool second_is_x = h == NOISE3_GRADIENT_CASE_X_A || h == NOISE3_GRADIENT_CASE_X_B;
    const float v = h < NOISE3_GRADIENT_SPLIT_V ? y : (second_is_x ? x : z);
    const float su = (h & NOISE3_GRADIENT_SIGN_U) != 0u ? -u : u;
    const float sv = (h & NOISE3_GRADIENT_SIGN_V) != 0u ? -v : v;
    return su + sv;
}

}  // namespace detail

/**
 * @brief Gradient noise at a point in three dimensions
 * @param p  sample point; one unit is one lattice cell
 * @return a value within plus or minus NOISE3_BOUND, exactly 0 on every lattice point
 */
[[nodiscard]] inline float perlin3(const Vec3& p) noexcept {
    const float fx = floor(p.x);
    const float fy = floor(p.y);
    const float fz = floor(p.z);
    const int32_t ix = static_cast<int32_t>(fx);
    const int32_t iy = static_cast<int32_t>(fy);
    const int32_t iz = static_cast<int32_t>(fz);

    // Offset inside the cell, [0, 1) on each axis, and the same offset seen from the far corner
    const float x = p.x - fx;
    const float y = p.y - fy;
    const float z = p.z - fz;
    const float x1 = x - 1.0f;
    const float y1 = y - 1.0f;
    const float z1 = z - 1.0f;

    const float u = smootherstep(x);
    const float v = smootherstep(y);
    const float w = smootherstep(z);

    const float n000 = detail::noise3_grad(detail::noise3_corner(ix, iy, iz), x, y, z);
    const float n100 = detail::noise3_grad(detail::noise3_corner(ix + 1, iy, iz), x1, y, z);
    const float n010 = detail::noise3_grad(detail::noise3_corner(ix, iy + 1, iz), x, y1, z);
    const float n110 = detail::noise3_grad(detail::noise3_corner(ix + 1, iy + 1, iz), x1, y1, z);
    const float n001 = detail::noise3_grad(detail::noise3_corner(ix, iy, iz + 1), x, y, z1);
    const float n101 = detail::noise3_grad(detail::noise3_corner(ix + 1, iy, iz + 1), x1, y, z1);
    const float n011 = detail::noise3_grad(detail::noise3_corner(ix, iy + 1, iz + 1), x, y1, z1);
    const float n111 = detail::noise3_grad(detail::noise3_corner(ix + 1, iy + 1, iz + 1), x1, y1, z1);

    const float nx00 = lerp(n000, n100, u);
    const float nx10 = lerp(n010, n110, u);
    const float nx01 = lerp(n001, n101, u);
    const float nx11 = lerp(n011, n111, u);
    const float nxy0 = lerp(nx00, nx10, v);
    const float nxy1 = lerp(nx01, nx11, v);
    return lerp(nxy0, nxy1, w);
}

/**
 * @brief Fractal sum of three-dimensional gradient noise in three decorrelated channels
 * @param p           sample point
 * @param octaves     number of layers; 0 or fewer yields the zero vector
 * @param rough       amplitude factor from one octave to the next
 * @param lacunarity  frequency factor from one octave to the next
 * @return per channel the amplitude-weighted mean of the octaves, so the range of perlin3 holds
 */
[[nodiscard]] inline Vec3 fbm3_vec(const Vec3& p, int octaves, float rough, float lacunarity) noexcept {
    Vec3 sum{};
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float total = 0.0f;
    for (int octave = 0; octave < octaves; ++octave) {
        const Vec3 q = p * frequency;
        sum.x += amplitude * perlin3(q);
        sum.y += amplitude * perlin3(q + NOISE3_CHANNEL_OFFSET_Y);
        sum.z += amplitude * perlin3(q + NOISE3_CHANNEL_OFFSET_Z);
        total += amplitude;
        amplitude *= rough;
        frequency *= lacunarity;
    }
    return total > 0.0f ? sum / total : Vec3{};
}

}  // namespace ase::math
