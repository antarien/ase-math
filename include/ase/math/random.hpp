#pragma once

/**
 * ASE MATH - RANDOM NUMBER GENERATION
 *
 * @file        random.hpp
 * @brief       The ASE-native random API used by every system
 * @description Deterministic-on-demand random floats, integers, booleans and signs. This is
 *              the contract every module, plugin and server calls; the generator itself and
 *              every use of the C++ random library live in random_std.hpp, which this header
 *              includes. Systems use math:: functions only and never reach for <random>.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @created     2026-01-11
 * @modified    2026-08-15
 * @version     2.0.0
 *
 * USAGE IN ECS SYSTEMS:
 *   #include <ase/math/random.hpp>
 *   float val = math::random_float();           // [0.0, 1.0)
 *   float range = math::random_float(0.5, 2.0); // [0.5, 2.0)
 *   int roll = math::random_int(0, 100);        // [0, 100]
 *   uint8_t type = math::random_uint8(0, 3);    // [0, 3]
 *
 * THREAD SAFETY:
 *   The generator behind these calls is per-thread — see random_std.hpp, where that property
 *   is stated and kept. Nothing here shares state between threads.
 */

// The delegation layer: generator, seeding and distributions.
#include <ase/math/random_std.hpp>
// sqrt, log, cos and TWO_PI for the normal draw of the seeded family
#include <ase/math/math.hpp>

#include <cstdint>

namespace ase::math {

/**
 * The constants of the seeded family below (random_hash_*). They are part of the contract: the
 * client port clients/ase-client-web/src/core/math/random.ts carries the same numbers, and a
 * changed value is a changed shape of every plant grown from a seed.
 */
constexpr uint32_t RANDOM_HASH_KEY_MUL = 0x9E3779B9u;   // golden-ratio multiplier, floor(2^32 / phi), spreads consecutive keys
constexpr uint32_t RANDOM_HASH_MIX_A = 0x85EBCA6Bu;     // first multiplier of the MurmurHash3 32-bit finaliser (fmix32)
constexpr uint32_t RANDOM_HASH_MIX_B = 0xC2B2AE35u;     // second multiplier of the MurmurHash3 32-bit finaliser (fmix32)
constexpr uint32_t RANDOM_HASH_SHIFT_A = 16u;           // first and last xor-shift of fmix32
constexpr uint32_t RANDOM_HASH_SHIFT_B = 13u;           // middle xor-shift of fmix32
constexpr uint32_t RANDOM_HASH_FLOAT_SHIFT = 8u;        // drops the low 8 bits: 24 bits fill the float mantissa exactly
constexpr float RANDOM_HASH_FLOAT_SCALE = 1.0f / 16777216.0f;  // 2^-24, maps the 24 kept bits onto [0, 1)
constexpr uint32_t RANDOM_HASH_NORMAL_KEYS = 2u;        // the normal draw consumes two keys: key * 2 and key * 2 + 1
constexpr float RANDOM_HASH_NORMAL_FLOOR = 1.0f / 16777216.0f;  // lower bound of u1 so log(u1) stays finite
constexpr float RANDOM_HASH_NORMAL_LOG_FACTOR = -2.0f;  // the -2 of the Box-Muller radius sqrt(-2 ln u1)

// =============================================================================
// RANDOM FLOAT FUNCTIONS
// =============================================================================

/**
 * @brief Generate random float in [0.0, 1.0)
 */
inline float random_float() {
    return detail::draw_float(0.0f, 1.0f);
}

/**
 * @brief Generate random float in [min, max)
 */
inline float random_float(float min_val, float max_val) {
    return detail::draw_float(min_val, max_val);
}

// =============================================================================
// RANDOM INTEGER FUNCTIONS
// =============================================================================

/**
 * @brief Generate random int in [min, max] (inclusive!)
 */
inline int random_int(int min_val, int max_val) {
    return detail::draw_int(min_val, max_val);
}

/**
 * @brief Generate random uint8_t in [min, max] (inclusive!)
 */
inline uint8_t random_uint8(uint8_t min_val, uint8_t max_val) {
    return static_cast<uint8_t>(detail::draw_int(min_val, max_val));
}

/**
 * @brief Generate random uint32_t in [min, max] (inclusive!)
 */
inline uint32_t random_uint32(uint32_t min_val, uint32_t max_val) {
    return detail::draw_uint32(min_val, max_val);
}

/**
 * @brief Generate random uint64_t (full range)
 */
inline uint64_t random_uint64() {
    return detail::draw_uint64();
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Seed the random generator (for deterministic testing)
 */
inline void random_seed(uint32_t seed) {
    detail::reseed(seed);
}

/**
 * @brief Random boolean with probability p for true
 * @param p Probability [0.0, 1.0] for returning true
 */
inline bool random_bool(float p = 0.5f) {
    return random_float() < p;
}

/**
 * @brief Random sign: returns -1.0f or 1.0f
 */
inline float random_sign() {
    return random_bool() ? 1.0f : -1.0f;
}

// =============================================================================
// SEEDED HASH FUNCTIONS - the same draw from the same seed on every platform
// =============================================================================
//
// WHY A SECOND FAMILY. The functions above draw from the per-thread std::mt19937 through the std
// distributions (random_std.hpp). The generator is specified by the standard, the distributions
// are not: libstdc++ on the Linux tiers and libc++ in the Android NDK map the same generator words
// to different floats, and the client port has no seedable generator at all. A shape that must
// come out the same from the same seed on server, handset and client - a plant grown in
// ase-phytomer (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.3, Schritt 6) - needs a draw that is plain
// integer arithmetic.
//
// THE DRAW IS A FUNCTION OF (seed, key) ALONE. There is no stream and no state to advance: the
// seed lives in a component of the thing that is drawn for, the key names the draw (the lineage of
// an organ, the cycle, the index of the draw). Two systems that run in another order, or on two
// threads, draw the same numbers - nothing is shared between calls.
//
// THE MIXER is the 32-bit finaliser of MurmurHash3 (fmix32, Austin Appleby, public domain), fed
// with the seed and the key times the golden-ratio multiplier (Knuth, TAOCP vol. 3, 6.4). Unsigned
// 32-bit arithmetic wraps by definition, so the word is the same on every compiler; the client
// port computes it with Math.imul and >>> 0.

/**
 * @brief The mixed 32-bit word of (seed, key)
 */
inline uint32_t random_hash_u32(uint32_t seed, uint32_t key) {
    uint32_t h = seed ^ (key * RANDOM_HASH_KEY_MUL);
    h ^= h >> RANDOM_HASH_SHIFT_A;
    h *= RANDOM_HASH_MIX_A;
    h ^= h >> RANDOM_HASH_SHIFT_B;
    h *= RANDOM_HASH_MIX_B;
    h ^= h >> RANDOM_HASH_SHIFT_A;
    return h;
}

/**
 * @brief Uniform float of (seed, key) in [0.0, 1.0) - 24 bits, exact in a float
 */
inline float random_hash_float(uint32_t seed, uint32_t key) {
    return static_cast<float>(random_hash_u32(seed, key) >> RANDOM_HASH_FLOAT_SHIFT) *
           RANDOM_HASH_FLOAT_SCALE;
}

/**
 * @brief Uniform float of (seed, key) in [min_val, max_val)
 */
inline float random_hash_range(uint32_t seed, uint32_t key, float min_val, float max_val) {
    return min_val + (max_val - min_val) * random_hash_float(seed, key);
}

/**
 * @brief Normal draw of (seed, key) with mean and standard deviation (Box-Muller)
 *
 * Consumes the two keys key * 2 and key * 2 + 1, so a caller that numbers its draws 0, 1, 2, ...
 * never reuses a uniform of another normal draw. u1 is held above zero, so the logarithm is
 * finite; the result is mean + deviation * sqrt(-2 ln u1) * cos(2 pi u2).
 */
inline float random_hash_normal(uint32_t seed, uint32_t key, float mean, float deviation) {
    const uint32_t base = key * RANDOM_HASH_NORMAL_KEYS;
    const float u1 = max(random_hash_float(seed, base), RANDOM_HASH_NORMAL_FLOOR);
    const float u2 = random_hash_float(seed, base + 1u);
    const float radius = sqrt(RANDOM_HASH_NORMAL_LOG_FACTOR * log(u1));
    return mean + deviation * radius * cos(TWO_PI * u2);
}

}  // namespace ase::math
