#pragma once

/**
 * ASE CORE INFRASTRUCTURE HEADER
 *
 * @file        math.hpp
 * @brief       ase::math - the engine's float math entry point and the one place std:: is called
 * @description The float transcendental and rounding functions of the engine, each a thin
 *              forward to the C++ standard library. This file exists so that no other file has
 *              to: modules, plugins and servers call ase::math::sin, ase::math::pow and their
 *              siblings, and <cmath> is reached through nothing else. The wrapper IS the
 *              relocation of the standard functions to a single place - it does not reimplement
 *              them, and nothing here should ever grow a hand-written substitute for one.
 *
 *              The engine's OWN scalar arithmetic - the angle constants and the generic
 *              abs/min/max/clamp/lerp/sign templates, which forward to nothing - moved to
 *              scalar.hpp on 2026-08-15, so that what the engine DEFINES and what it FORWARDS
 *              are two files. scalar.hpp is included below, so every existing consumer keeps
 *              finding all of ase::math through this header without changing a line.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @created     2025-12-15
 * @modified    2026-08-15
 * @version     2.0.0
 *
 * CORE INFRASTRUCTURE COMPLIANCE
 *
 * [ ] NOT an ECS Component or System
 * [ ] Layer dependencies correct (L0: no ASE deps, L1: L0 only)
 * [ ] No global mutable state (constexpr/const only)
 * [ ] No singletons or static mutable variables
 * [ ] Thread-safe by design (pure functions or explicit mutex)
 * [ ] All public functions documented with @brief, @param, @return
 * [ ] constexpr where possible (compile-time evaluation)
 * [ ] noexcept where possible (no-throw guarantee)
 * [ ] [[nodiscard]] on functions returning values
 * [ ] No magic numbers (use named constants)
 * [ ] No implicit conversions (use explicit constructors)
 * [ ] Header-only OR header+cpp pattern (not mixed)
 * [ ] Include guards via #pragma once
 * [ ] Namespace matches module: ase::{module}
 * [ ] No circular dependencies
 * [ ] No macros (except include guards) - use constexpr/templates
 * [ ] API stable (changes require version bump)
 */

#include <cmath>

// The engine's own scalar arithmetic. Included here on purpose: consumers include math.hpp.
#include <ase/math/scalar.hpp>

namespace ase::math {

// =============================================================================
// FLOATING POINT FUNCTIONS - the delegation layer to <cmath>
// =============================================================================

inline float sqrt(float value) {
    return std::sqrt(value);
}

inline float sin(float value) {
    return std::sin(value);
}

inline float cos(float value) {
    return std::cos(value);
}

inline float tan(float value) {
    return std::tan(value);
}

inline float asin(float value) {
    return std::asin(value);
}

inline float acos(float value) {
    return std::acos(value);
}

inline float atan2(float y, float x) {
    return std::atan2(y, x);
}

inline float fmod(float x, float y) {
    return std::fmod(x, y);
}

inline float floor(float value) {
    return std::floor(value);
}

inline float ceil(float value) {
    return std::ceil(value);
}

inline float round(float value) {
    return std::round(value);
}

inline float pow(float base, float exp) {
    return std::pow(base, exp);
}

inline float exp(float value) {
    return std::exp(value);
}

inline float log(float value) {
    return std::log(value);
}

}  // namespace ase::math
