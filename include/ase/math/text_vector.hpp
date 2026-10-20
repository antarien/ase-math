#pragma once

/**
 * ASE MATH - TEXT VECTORS (LOCAL EMBEDDING)
 *
 * @file        text_vector.hpp
 * @brief       SSOT for the local embedding: a deterministic feature-hashing vector of a text, and its cosine
 * @description PLAN_ASE_REASONING_PHASE_18 Task 18.4 makes a LOCAL embedding a hard dependency: the novelty
 *              score of admission, the key of the semantic cache and the embedding_distance signal of
 *              resolution step 0 all need a vector for a text, and computing it with a provider would turn
 *              the zero-egress path into a metered call. This header is that local embedder - pure
 *              functions, no model file, no state, no egress: the same bytes give the same vector in every
 *              process, so a vector stored by one shard is comparable with one computed by another. ONE
 *              implementation here, beside polygon.hpp and interpolation.hpp - no copy in a consumer. The
 *              square root comes through math.hpp, the one place <cmath> is reached.
 *
 *              THE FEATURES. Every maximal run of word bytes (ASCII letters and digits, and every byte of a
 *              multi-byte UTF-8 sequence, so a non-ASCII word stays whole) is one word feature, case-folded.
 *              Every three consecutive bytes of the text with each gap of non-word bytes folded to one
 *              space is one trigram feature at half the weight - the trigrams let two spellings of a word
 *              and two word orders lie close without being equal. Each feature is hashed (FNV-1a with a
 *              final avalanche) to one of `dims` positions and a sign; the sum is scaled to unit length.
 *              Two texts sharing most of their words and trigrams have a cosine near 1, unrelated texts a
 *              cosine near 0 - a lexical similarity, which is what reuse of an answer to the same
 *              situation needs.
 *
 *              THE DIMENSION. TEXT_VECTOR_DIMS = 1536 is the dimension of the skill-memory vector index
 *              (Phase 3, MEM_VECTOR_DIMS), so a local vector fits the existing Atlas index of
 *              skill_inference_cache without a second index shape.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @created     2026-10-07
 * @modified    2026-10-07
 * @version     1.0.0
 *
 * USAGE:
 *   #include <ase/math/text_vector.hpp>
 *   float v[math::TEXT_VECTOR_DIMS];
 *   math::text_vector_fill(text, len, v, math::TEXT_VECTOR_DIMS);
 *   float sim = math::text_vector_cosine(v, w, math::TEXT_VECTOR_DIMS);   // [-1, 1]
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

#include <ase/math/math.hpp>

#include <cstdint>

namespace ase::math {

constexpr uint32_t TEXT_VECTOR_DIMS        = 1536u;        // dimension of the skill-memory vector index (Phase 3 MEM_VECTOR_DIMS)
constexpr float    TEXT_VECTOR_WORD_WEIGHT = 1.0f;         // weight of one word feature
constexpr float    TEXT_VECTOR_GRAM_WEIGHT = 0.5f;         // weight of one trigram feature
constexpr float    TEXT_VECTOR_NORM_EPS    = 1.0e-12f;     // squared length below which a vector counts as zero
constexpr uint32_t TEXT_VECTOR_FNV_OFFSET  = 2166136261u;  // FNV-1a 32 offset basis
constexpr uint32_t TEXT_VECTOR_FNV_PRIME   = 16777619u;    // FNV-1a 32 prime
constexpr uint32_t TEXT_VECTOR_GRAM_SEED   = 0x9E3779B9u;  // start value of a trigram hash - keeps a trigram apart from a three-letter word
constexpr uint32_t TEXT_VECTOR_MIX_A       = 0x7FEB352Du;  // first multiplier of the final avalanche
constexpr uint32_t TEXT_VECTOR_MIX_B       = 0x846CA68Bu;  // second multiplier of the final avalanche
constexpr uint8_t  TEXT_VECTOR_SPACE       = 0x20u;        // the byte a gap of non-word bytes folds to
constexpr uint8_t  TEXT_VECTOR_MULTIBYTE   = 0x80u;        // first byte value of a multi-byte UTF-8 sequence
constexpr uint8_t  TEXT_VECTOR_CASE_GAP    = 0x20u;        // distance between an ASCII upper- and lower-case letter
constexpr float    TEXT_VECTOR_SIGN_POS    = 1.0f;         // sign of a feature whose avalanche top bit is clear
constexpr float    TEXT_VECTOR_SIGN_NEG    = -1.0f;        // sign of a feature whose avalanche top bit is set

namespace detail {

// True for a byte that belongs to a word: ASCII letter or digit, or any byte of a multi-byte sequence.
[[nodiscard]] constexpr bool text_vector_word_byte(uint8_t c) noexcept {
    return (c >= static_cast<uint8_t>('0') && c <= static_cast<uint8_t>('9')) ||
           (c >= static_cast<uint8_t>('a') && c <= static_cast<uint8_t>('z')) ||
           (c >= static_cast<uint8_t>('A') && c <= static_cast<uint8_t>('Z')) || c >= TEXT_VECTOR_MULTIBYTE;
}

// The byte case-folded: an ASCII capital becomes its small letter, every other byte stays.
[[nodiscard]] constexpr uint8_t text_vector_fold(uint8_t c) noexcept {
    const bool capital = c >= static_cast<uint8_t>('A') && c <= static_cast<uint8_t>('Z');
    return capital ? static_cast<uint8_t>(c + TEXT_VECTOR_CASE_GAP) : c;
}

// One FNV-1a step.
[[nodiscard]] constexpr uint32_t text_vector_step(uint32_t h, uint8_t c) noexcept {
    return (h ^ static_cast<uint32_t>(c)) * TEXT_VECTOR_FNV_PRIME;
}

// Add one feature: the avalanche spreads neighbouring hashes, the low bits pick the position, the top bit
// the sign - signed hashing keeps the expected inner product of two unrelated texts at zero.
inline void text_vector_add(float* out, uint32_t dims, uint32_t h, float weight) noexcept {
    h ^= h >> 16;
    h *= TEXT_VECTOR_MIX_A;
    h ^= h >> 15;
    h *= TEXT_VECTOR_MIX_B;
    h ^= h >> 16;
    const uint32_t index = h % dims;
    const float sign = ((h >> 31) & 1u) != 0u ? TEXT_VECTOR_SIGN_NEG : TEXT_VECTOR_SIGN_POS;
    out[index] += sign * weight;
}

}  // namespace detail

/**
 * @brief Fill out[0..dims) with the unit feature-hash vector of text[0..len).
 * @param text  the bytes to embed (need not be NUL-terminated)
 * @param len   number of bytes of text
 * @param out   caller-owned buffer of dims floats
 * @param dims  vector dimension (TEXT_VECTOR_DIMS for the skill-memory index)
 * @return the number of features hashed; 0 when the text holds no word byte, and out is all zero then
 */
[[nodiscard]] inline uint32_t text_vector_fill(const char* text, uint32_t len, float* out, uint32_t dims) noexcept {
    if (out == nullptr || dims == 0u) return 0u;
    for (uint32_t i = 0u; i < dims; ++i) out[i] = 0.0f;
    if (text == nullptr || len == 0u) return 0u;

    uint32_t features = 0u;

    // WORDS - every maximal run of word bytes, case-folded.
    uint32_t at = 0u;
    while (at < len) {
        while (at < len && !detail::text_vector_word_byte(static_cast<uint8_t>(text[at]))) ++at;
        if (at >= len) break;
        uint32_t h = TEXT_VECTOR_FNV_OFFSET;
        while (at < len && detail::text_vector_word_byte(static_cast<uint8_t>(text[at]))) {
            h = detail::text_vector_step(h, detail::text_vector_fold(static_cast<uint8_t>(text[at])));
            ++at;
        }
        detail::text_vector_add(out, dims, h, TEXT_VECTOR_WORD_WEIGHT);
        ++features;
    }
    if (features == 0u) return 0u;

    // TRIGRAMS - over the case-folded bytes, every gap of non-word bytes folded to one space.
    uint8_t first = TEXT_VECTOR_SPACE;
    uint8_t second = TEXT_VECTOR_SPACE;
    uint32_t taken = 0u;
    bool in_gap = true;
    for (uint32_t i = 0u; i < len; ++i) {
        const uint8_t raw = static_cast<uint8_t>(text[i]);
        const bool word = detail::text_vector_word_byte(raw);
        if (!word && in_gap) continue;
        in_gap = !word;
        const uint8_t next = word ? detail::text_vector_fold(raw) : TEXT_VECTOR_SPACE;
        if (taken >= 2u) {
            uint32_t h = TEXT_VECTOR_GRAM_SEED;
            h = detail::text_vector_step(h, first);
            h = detail::text_vector_step(h, second);
            h = detail::text_vector_step(h, next);
            detail::text_vector_add(out, dims, h, TEXT_VECTOR_GRAM_WEIGHT);
            ++features;
        }
        first = second;
        second = next;
        ++taken;
    }

    // UNIT LENGTH - the cosine of two filled vectors is then their inner product.
    float length_sq = 0.0f;
    for (uint32_t i = 0u; i < dims; ++i) length_sq += out[i] * out[i];
    if (length_sq < TEXT_VECTOR_NORM_EPS) return features;
    const float inverse = 1.0f / math::sqrt(length_sq);
    for (uint32_t i = 0u; i < dims; ++i) out[i] *= inverse;
    return features;
}

/**
 * @brief Cosine similarity of two vectors of dims floats.
 * @param a     first vector (dims floats)
 * @param b     second vector (dims floats)
 * @param dims  vector dimension
 * @return the cosine in [-1, 1]; 0 when either vector is the zero vector or missing
 */
[[nodiscard]] inline float text_vector_cosine(const float* a, const float* b, uint32_t dims) noexcept {
    if (a == nullptr || b == nullptr) return 0.0f;
    float dot = 0.0f;
    float a_sq = 0.0f;
    float b_sq = 0.0f;
    for (uint32_t i = 0u; i < dims; ++i) {
        dot += a[i] * b[i];
        a_sq += a[i] * a[i];
        b_sq += b[i] * b[i];
    }
    if (a_sq < TEXT_VECTOR_NORM_EPS || b_sq < TEXT_VECTOR_NORM_EPS) return 0.0f;
    return dot / math::sqrt(a_sq * b_sq);
}

}  // namespace ase::math
