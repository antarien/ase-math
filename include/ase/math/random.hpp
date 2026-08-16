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

#include <cstdint>

namespace ase::math {

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

}  // namespace ase::math
