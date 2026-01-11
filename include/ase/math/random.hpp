#pragma once

/**
 * ASE MATH - RANDOM NUMBER GENERATION
 *
 * @file        random.hpp
 * @brief       Thread-safe random number generation for ECS systems
 * @description Provides deterministic and thread-safe random functions.
 *              Wraps std::random internally - systems use math:: functions only!
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @created     2026-01-11
 * @modified    2026-01-11
 * @version     1.0.0
 *
 * USAGE IN ECS SYSTEMS:
 *   #include <ase/math/random.hpp>
 *   float val = math::random_float();           // [0.0, 1.0)
 *   float range = math::random_float(0.5, 2.0); // [0.5, 2.0)
 *   int roll = math::random_int(0, 100);        // [0, 100]
 *   uint8_t type = math::random_uint8(0, 3);    // [0, 3]
 *
 * THREAD SAFETY:
 *   All functions use thread_local RNG - safe for parallel systems!
 */

#include <random>
#include <cstdint>

namespace ase::math {

namespace detail {

// Thread-local random generator (hidden from ECS code)
inline thread_local std::mt19937 g_rng{std::random_device{}()};

}  // namespace detail

// =============================================================================
// RANDOM FLOAT FUNCTIONS
// =============================================================================

/**
 * @brief Generate random float in [0.0, 1.0)
 */
inline float random_float() {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(detail::g_rng);
}

/**
 * @brief Generate random float in [min, max)
 */
inline float random_float(float min_val, float max_val) {
    std::uniform_real_distribution<float> dist(min_val, max_val);
    return dist(detail::g_rng);
}

// =============================================================================
// RANDOM INTEGER FUNCTIONS
// =============================================================================

/**
 * @brief Generate random int in [min, max] (inclusive!)
 */
inline int random_int(int min_val, int max_val) {
    std::uniform_int_distribution<int> dist(min_val, max_val);
    return dist(detail::g_rng);
}

/**
 * @brief Generate random uint8_t in [min, max] (inclusive!)
 */
inline uint8_t random_uint8(uint8_t min_val, uint8_t max_val) {
    std::uniform_int_distribution<int> dist(min_val, max_val);
    return static_cast<uint8_t>(dist(detail::g_rng));
}

/**
 * @brief Generate random uint32_t in [min, max] (inclusive!)
 */
inline uint32_t random_uint32(uint32_t min_val, uint32_t max_val) {
    std::uniform_int_distribution<uint32_t> dist(min_val, max_val);
    return dist(detail::g_rng);
}

/**
 * @brief Generate random uint64_t (full range)
 */
inline uint64_t random_uint64() {
    std::uniform_int_distribution<uint64_t> dist;
    return dist(detail::g_rng);
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

/**
 * @brief Seed the random generator (for deterministic testing)
 */
inline void random_seed(uint32_t seed) {
    detail::g_rng.seed(seed);
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
