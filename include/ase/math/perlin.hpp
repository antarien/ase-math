#pragma once

/**
 * Perlin Noise - 2D implementation
 *
 * Header-only, deterministic, suitable for terrain generation.
 * Based on Ken Perlin's improved noise algorithm.
 */

#include <cmath>
#include <cstdint>
#include <array>

namespace ase::math {

class Perlin {
public:
    /**
     * Create Perlin noise generator with seed
     */
    explicit Perlin(uint32_t seed = 0) {
        // Initialize permutation table
        for (int i = 0; i < 256; ++i) {
            perm_[i] = static_cast<uint8_t>(i);
        }

        // Shuffle using seed
        uint32_t s = seed;
        for (int i = 255; i > 0; --i) {
            s = lcg(s);
            int j = s % (i + 1);
            std::swap(perm_[i], perm_[j]);
        }

        // Duplicate for overflow
        for (int i = 0; i < 256; ++i) {
            perm_[256 + i] = perm_[i];
        }
    }

    /**
     * 2D Perlin noise at position (x, y)
     * Returns value in range [-1, 1]
     */
    float noise(float x, float y) const {
        // Find unit grid cell containing point
        int xi = static_cast<int>(std::floor(x)) & 255;
        int yi = static_cast<int>(std::floor(y)) & 255;

        // Relative position within cell
        float xf = x - std::floor(x);
        float yf = y - std::floor(y);

        // Fade curves
        float u = fade(xf);
        float v = fade(yf);

        // Hash coordinates of 4 corners
        int aa = perm_[perm_[xi] + yi];
        int ab = perm_[perm_[xi] + yi + 1];
        int ba = perm_[perm_[xi + 1] + yi];
        int bb = perm_[perm_[xi + 1] + yi + 1];

        // Gradient values at corners
        float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1, yf), u);
        float x2 = lerp(grad(ab, xf, yf - 1), grad(bb, xf - 1, yf - 1), u);

        return lerp(x1, x2, v);
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
    std::array<uint8_t, 512> perm_;

    // Linear congruential generator for seeding
    static uint32_t lcg(uint32_t x) {
        return x * 1103515245 + 12345;
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
        int h = hash & 7;
        float u = h < 4 ? x : y;
        float v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }
};

}  // namespace ase::math
