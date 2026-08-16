#pragma once

/**
 * ASE FOUNDATION HEADER
 *
 * @file        random_std.hpp
 * @brief       The single place in the engine where the C++ random library is used
 * @description Holds the Mersenne Twister generator, its seeding, and the distribution calls
 *              that draw from it. Nothing else in the tree touches <random>: modules, plugins
 *              and servers call the ase::math::random_* functions in random.hpp, which come
 *              through the four primitives below. Splitting the wrapper out of random.hpp
 *              separates the ASE-NATIVE API from the delegation, so a reader can tell which
 *              half is the engine's own contract and which half is standard library.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @created     2026-08-15
 * @modified    2026-08-15
 * @version     1.0.0
 *
 * THREAD SAFETY IS A PROPERTY OF THIS FILE, NOT A FORMALITY
 *
 * The generator is thread_local, and it stays that way. It is drawn from by whatever thread
 * happens to be running the caller — schedules, the httplib worker threads in the webserver
 * plugins, tooling — and a shared mt19937 without a lock would be a data race, not a style
 * question. Removing the specifier would not make the engine single-threaded, it would make
 * the RNG unsafe in the places that already are not.
 *
 * WHY THE VALIDATOR STILL REPORTS THIS FILE
 *
 * The std::random ban and the thread_local ban carry no path exclusion, so they fire here —
 * on the definition of the alternative they point callers to, and on the property that makes
 * it safe. That is expected and it is the point of the split: the reports are confined to the
 * file that exists to hold them. Whether the bans should gain an exclusion for this path is an
 * operator decision, not a repair.
 */

#include <random>
#include <cstdint>

namespace ase::math::detail {

/** The one generator. Per-thread, so two threads never share its state. */
inline thread_local std::mt19937 g_rng{std::random_device{}()};

/** Uniform float in [min_val, max_val). */
inline float draw_float(float min_val, float max_val) {
    std::uniform_real_distribution<float> dist(min_val, max_val);
    return dist(g_rng);
}

/** Uniform int in [min_val, max_val], inclusive. */
inline int draw_int(int min_val, int max_val) {
    std::uniform_int_distribution<int> dist(min_val, max_val);
    return dist(g_rng);
}

/** Uniform uint32 in [min_val, max_val], inclusive. */
inline uint32_t draw_uint32(uint32_t min_val, uint32_t max_val) {
    std::uniform_int_distribution<uint32_t> dist(min_val, max_val);
    return dist(g_rng);
}

/** Uniform uint64 over the full range. */
inline uint64_t draw_uint64() {
    std::uniform_int_distribution<uint64_t> dist;
    return dist(g_rng);
}

/** Reseed this thread's generator (deterministic testing). */
inline void reseed(uint32_t seed) {
    g_rng.seed(seed);
}

}  // namespace ase::math::detail
