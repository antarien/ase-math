#pragma once

/**
 * ASE Math Reference Ellipsoid Formula Building Blocks
 *
 * @file        hexgrid.hpp
 * @brief       Formula building blocks of the reference ellipsoid (SSOT)
 * @description Layer 0 foundation surface for everything that reads a direction against the
 *              reference ellipsoid: its surface area, the conversion between a geocentric
 *              direction and a geodetic latitude and longitude, the point where a direction
 *              pierces the surface, and the rotation of a vector about an axis.
 *
 *              THIS HEADER CARRIES NO LATTICE CONSTRUCTION. The planetary hexagon lattice -
 *              icosahedron base, cell addressing, cell ring, neighbourhood, edges, epoch
 *              arithmetic and the pentagon predicate - lives in `modules/ase-geoid` as SHARED
 *              Components plus Systems (PLAN_ASE_LATTICE.md, Festlegung 6 in ihrer Fassung vom
 *              2026-08-06). The reason is structural, not a matter of taste: `ase-codegen` is the
 *              mechanism that keeps C++/EnTT and TypeScript/becsy at 1:1 parity, it scans
 *              `modules/` and `plugins/`, and `foundation/` carries no `codegen.json`. A
 *              construction placed here is unreachable for the transpiler, and the client can only
 *              get at it through a hand written second version - which is exactly what
 *              INST_ASE_CREATE_MOD.md lists as Fehler 1 and Fehler 2.
 *
 *              What remains here is what `spherical.hpp` is: pure vector and ellipsoid formulas,
 *              header inline, allocation free, with no ECS and no state. Every entry point takes
 *              its ellipsoid as an argument, so no planet is ever baked in.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @created     2026-07-30
 * @modified    2026-08-06
 * @version     2.0.0
 *
 * DRY / SOLID / SSOT COMPLIANCE:
 * - NO ECS: no registry, no EnTT, no component types in this header
 * - NO allocation: every function returns by value or writes to caller supplied references
 * - NO lattice construction: the honeycomb belongs to modules/ase-geoid
 * - Spherical and cartesian conversions come from spherical.hpp, they are never restated here
 */

#include <ase/math/math.hpp>
#include <ase/math/spherical.hpp>
#include <ase/math/vec3.hpp>

namespace ase::math {

// =============================================================================
// NUMERIC GUARDS OF THE ELLIPSOID FORMULAS
// =============================================================================

/**
 * Largest flattening the formulas below accept. Above it the polar radius would collapse onto the
 * centre and every quotient in this header would lose its meaning; the value is a guard on the
 * arithmetic, not a statement about any planet.
 */
constexpr float HEXGRID_FLATTENING_MAX = 0.999f;

/**
 * Squared eccentricity below which the ellipsoid is treated as a sphere. Below this the series
 * behind the area formula is dominated by its rounding error, so the closed sphere form is both
 * faster and more accurate.
 */
constexpr float HEXGRID_ECCENTRICITY_EPSILON = 1.0e-9f;

// =============================================================================
// REFERENCE ELLIPSOID PARAMETERS
// =============================================================================

/**
 * The reference ellipsoid is the geodetic base surface of every coordinate system
 * (DSGN_118_DIMENSIONS_SCALES_METRICS.md, section 91.3, Z. 120-122). That section carries no
 * numeric ellipsoid parameters, so the parameters are function INPUT here and are never baked in.
 *
 * THEY TRAVEL AS TWO SCALARS, NOT AS A STRUCT. semi_major_axis_m is the equatorial radius in
 * metres, flattening is the dimensionless (a - b) / a, and a flattening of zero degenerates the
 * ellipsoid into a sphere - legal input. The scalar form is what `spherical.hpp` uses for its own
 * parameters, and it is what lets a SHARED system of `modules/ase-geoid` call these formulas
 * without declaring a local struct: `ase-codegen` transpiles the call, it does not transpile a
 * C++ struct declaration.
 *
 * Note which parameter each entry point actually needs. The geodetic conversions take ONLY the
 * flattening, because a direction has no size - that is why a lattice anchor survives geoid
 * growth unchanged. The equatorial radius appears in exactly two places: the surface area and the
 * surface point.
 */

// =============================================================================
// VECTOR FORMULA BUILDING BLOCKS
// =============================================================================

/**
 * @brief Rodrigues rotation of a vector about a unit axis
 * @param value Vector to rotate
 * @param axis Unit rotation axis
 * @param angle Rotation angle in radians, right handed about the axis
 * @return The rotated vector
 *
 * The closed form of a rotation without a matrix: the component along the axis is kept, the
 * perpendicular component is turned in its own plane.
 */
inline Vec3 hexgrid_rotate_axis(const Vec3& value, const Vec3& axis, float angle) {
    const float cos_a = cos(angle);
    const float sin_a = sin(angle);
    return value * cos_a + cross(axis, value) * sin_a + axis * (dot(axis, value) * (1.0f - cos_a));
}

// =============================================================================
// ELLIPSOID FORMULA BUILDING BLOCKS
// =============================================================================

/**
 * @brief Surface area of the reference ellipsoid in square metres
 * @param ellipsoid Reference ellipsoid parameters
 * @return Surface area A(R) in square metres, zero for a degenerate ellipsoid
 *
 * The closed form of an oblate spheroid: two pi a squared times one plus the inverse eccentricity
 * times the area hyperbolic tangent of the eccentricity. At vanishing eccentricity it collapses
 * into the sphere formula, which is the branch taken below.
 */
inline float hexgrid_ellipsoid_surface_area(float semi_major_axis_m, float flattening_in) {
    const float semi_major = semi_major_axis_m;
    if (semi_major <= 0.0f) {
        return 0.0f;
    }
    const float flattening = clamp(flattening_in, 0.0f, HEXGRID_FLATTENING_MAX);
    const float eccentricity_sq = 2.0f * flattening - flattening * flattening;
    if (eccentricity_sq < HEXGRID_ECCENTRICITY_EPSILON) {
        return 4.0f * PI * semi_major * semi_major;
    }
    const float eccentricity = sqrt(eccentricity_sq);
    const float area_tanh = 0.5f * log((1.0f + eccentricity) / (1.0f - eccentricity));
    return 2.0f * PI * semi_major * semi_major *
           (1.0f + ((1.0f - eccentricity_sq) / eccentricity) * area_tanh);
}

/**
 * @brief Geodetic latitude of a geocentric unit direction
 * @param direction Unit direction on the sphere, Y up
 * @param flattening Flattening of the reference ellipsoid, 0 for a sphere
 * @return Geodetic latitude in degrees, -90 to 90
 *
 * Only the flattening enters, because that is what separates geodetic from geocentric latitude.
 * The equatorial radius does not appear at all - a direction has no size.
 *
 * Latitude and longitude are two functions rather than one with output parameters: an output
 * parameter has no counterpart in the transpiled client, where the write would be lost silently
 * and every anchor would read zero.
 */
inline float hexgrid_direction_latitude(const Vec3& direction, float flattening) {
    const Spherical spherical = cartesian_to_spherical(direction);
    const float geocentric_lat = HALF_PI - spherical.theta;
    const float polar_ratio = 1.0f - clamp(flattening, 0.0f, HEXGRID_FLATTENING_MAX);
    const float squashed = polar_ratio * polar_ratio;
    const float geodetic_lat = atan2(sin(geocentric_lat), squashed * cos(geocentric_lat));
    return geodetic_lat * RAD_TO_DEG;
}

/**
 * @brief Geodetic longitude of a geocentric unit direction
 * @param direction Unit direction on the sphere, Y up
 * @return Geodetic longitude in degrees, -180 to 180
 *
 * The flattening does not enter: a rotation about the polar axis leaves the meridian where it is.
 */
inline float hexgrid_direction_longitude(const Vec3& direction) {
    const Spherical spherical = cartesian_to_spherical(direction);
    float longitude = spherical.phi;
    if (longitude > PI) {
        longitude -= TWO_PI;
    }
    return longitude * RAD_TO_DEG;
}

/**
 * @brief Geocentric unit direction of a geodetic latitude and longitude
 * @param latitude_deg Geodetic latitude in degrees, -90 to 90
 * @param longitude_deg Geodetic longitude in degrees, -180 to 180
 * @param ellipsoid Reference ellipsoid parameters
 * @return Unit direction on the sphere, Y up
 *
 * Exact inverse of hexgrid_direction_to_geodetic. Longitude is wrapped into [0, 2 pi) before the
 * spherical conversion, so a caller may hand in any winding of the same meridian.
 */
inline Vec3 hexgrid_geodetic_to_direction(float latitude_deg, float longitude_deg,
                                          float flattening) {
    const float geodetic_lat = clamp(latitude_deg, -90.0f, 90.0f) * DEG_TO_RAD;
    const float polar_ratio = 1.0f - clamp(flattening, 0.0f, HEXGRID_FLATTENING_MAX);
    const float squashed = polar_ratio * polar_ratio;
    const float geocentric_lat = atan2(squashed * sin(geodetic_lat), cos(geodetic_lat));
    const float theta = HALF_PI - geocentric_lat;
    float phi = longitude_deg * DEG_TO_RAD;
    while (phi < 0.0f) {
        phi += TWO_PI;
    }
    while (phi >= TWO_PI) {
        phi -= TWO_PI;
    }
    return spherical_to_cartesian(phi, theta, 1.0f);
}

/**
 * @brief Point where a geocentric direction pierces the ellipsoid surface
 * @param direction Unit direction on the sphere, Y up
 * @param ellipsoid Reference ellipsoid parameters
 * @return Cartesian point on the surface in metres, Y up, zero for a degenerate ellipsoid
 *
 * This is the one place the equatorial radius enters: it turns a direction into a position. A
 * consumer that holds a lattice corner as a direction scales it to the surface through here.
 */
inline Vec3 hexgrid_surface_point(const Vec3& direction, float semi_major_axis_m,
                                  float flattening) {
    const Spherical spherical = cartesian_to_spherical(direction);
    const float semi_major = semi_major_axis_m;
    const float semi_minor =
        semi_major * (1.0f - clamp(flattening, 0.0f, HEXGRID_FLATTENING_MAX));
    if (semi_major <= 0.0f) {
        return Vec3::zero();
    }
    if (semi_minor <= 0.0f) {
        return Vec3::zero();
    }
    const float sin_theta = sin(spherical.theta);
    const float cos_theta = cos(spherical.theta);
    const float inverse = sqrt((sin_theta * sin_theta) / (semi_major * semi_major) +
                               (cos_theta * cos_theta) / (semi_minor * semi_minor));
    const float radius = (inverse > 0.0f) ? (1.0f / inverse) : 0.0f;
    return spherical_to_cartesian(spherical.phi, spherical.theta, radius);
}

}  // namespace ase::math
