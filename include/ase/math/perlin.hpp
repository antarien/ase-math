#pragma once

/**
 * ASE FOUNDATION HEADER
 *
 * @file        perlin.hpp
 * @brief       Perlin - deterministic 2D gradient noise with fBm layering
 * @description Ken Perlin's improved noise in two dimensions, seeded through a small LCG so the
 *              same seed always yields the same field. Suitable for terrain generation. Holds a
 *              512-entry permutation table as a plain C array; floor goes through
 *              ase::math::floor rather than std::floor.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @created     2025-12-16
 * @modified    2026-08-15
 * @version     1.0.0
 */

#include <ase/math/math.hpp>

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

}  // namespace ase::math
