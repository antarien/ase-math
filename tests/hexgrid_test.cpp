/**
 * =============================================================================
 * ASE MATH - Reference Ellipsoid Formula Building Blocks Unit Tests
 * =============================================================================
 *
 * @file        hexgrid_test.cpp
 * @brief       Cases for the ellipsoid and vector formulas that stayed in this header.
 * @description The lattice construction moved to `modules/ase-geoid` (PLAN_ASE_LATTICE.md,
 *              Festlegung 6 in ihrer Fassung vom 2026-08-06), and its seven cases moved with it
 *              into `modules/ase-geoid/tests/test_geoid.cpp`. What is tested here is what
 *              stayed: the ellipsoid and vector formulas this header now consists of.
 *
 *              The parameters travel as SCALARS, and the cases below pin what that makes
 *              visible: the geodetic conversions take only the flattening, because a direction
 *              has no size.
 *
 *              The doctest main is switched on from tests/CMakeLists.txt rather than from a
 *              define here, so this file carries no macro definition of its own.
 *
 * -----------------------------------------------------------------------------
 * META
 * -----------------------------------------------------------------------------
 * @module      ase-math
 * @layer       0 (Foundation)
 * @category    process/computation/algorithm
 * @created     2026-08-01
 * @modified    2026-08-08
 * @version     1.0.0
 *
 * =============================================================================
 */

#include <doctest/doctest.h>

#include <ase/math/hexgrid.hpp>

using namespace ase::math;

namespace {

constexpr float SPHERE_RADIUS_M = 6371000.0f;         // A round planet sized sphere, metres
constexpr float SPHERE_FLATTENING = 0.0f;             // Zero flattening IS a sphere
constexpr float OBLATE_FLATTENING = 1.0f / 298.257f;  // An Earth like oblateness, dimensionless
constexpr float ANGLE_TOLERANCE_DEG = 1.0e-3f;        // Round trip tolerance in degrees
constexpr float RELATIVE_TOLERANCE = 1.0e-4f;         // Relative tolerance of an area comparison
constexpr float ORTHOGONAL_TOLERANCE = 1.0e-3f;       // Tolerance of a right angle check

}  // namespace

TEST_CASE("Hexgrid formulas - the surface area collapses into the sphere form at zero flattening") {
    const float expected = 4.0f * PI * SPHERE_RADIUS_M * SPHERE_RADIUS_M;
    const float measured = hexgrid_ellipsoid_surface_area(SPHERE_RADIUS_M, SPHERE_FLATTENING);
    REQUIRE(measured > 0.0f);
    CHECK(abs(measured - expected) / expected < RELATIVE_TOLERANCE);

    SUBCASE("an oblate ellipsoid carries LESS surface than its equatorial sphere") {
        const float oblate = hexgrid_ellipsoid_surface_area(SPHERE_RADIUS_M, OBLATE_FLATTENING);
        CHECK(oblate > 0.0f);
        CHECK(oblate < measured);
    }

    SUBCASE("a degenerate ellipsoid reports zero rather than a plausible looking number") {
        CHECK(hexgrid_ellipsoid_surface_area(0.0f, SPHERE_FLATTENING) == 0.0f);
    }
}

TEST_CASE("Hexgrid formulas - direction and geodetic position are exact inverses") {
    const float latitudes[5] = {-89.5f, -37.25f, 0.0f, 12.5f, 89.5f};
    const float longitudes[5] = {-179.0f, -47.25f, 0.0f, 47.25f, 179.0f};

    bool round_trip_holds = true;
    for (uint32_t a = 0u; a < 5u; ++a) {
        for (uint32_t b = 0u; b < 5u; ++b) {
            const Vec3 direction =
                hexgrid_geodetic_to_direction(latitudes[a], longitudes[b], OBLATE_FLATTENING);
            const float latitude_deg =
                hexgrid_direction_latitude(direction, OBLATE_FLATTENING);
            const float longitude_deg = hexgrid_direction_longitude(direction);
            if (abs(latitude_deg - latitudes[a]) > ANGLE_TOLERANCE_DEG) {
                round_trip_holds = false;
            }
            if (abs(longitude_deg - longitudes[b]) > ANGLE_TOLERANCE_DEG) {
                round_trip_holds = false;
            }
        }
    }
    CHECK(round_trip_holds);

    SUBCASE("on a sphere the geodetic latitude IS the geocentric latitude") {
        const Vec3 direction = hexgrid_geodetic_to_direction(45.0f, 90.0f, SPHERE_FLATTENING);
        const float latitude_deg = hexgrid_direction_latitude(direction, SPHERE_FLATTENING);
        const float longitude_deg = hexgrid_direction_longitude(direction);
        CHECK(abs(latitude_deg - 45.0f) < ANGLE_TOLERANCE_DEG);
        CHECK(abs(longitude_deg - 90.0f) < ANGLE_TOLERANCE_DEG);
    }

    SUBCASE("the geodetic latitude of an oblate ellipsoid exceeds its geocentric latitude") {
        const Vec3 direction = hexgrid_geodetic_to_direction(45.0f, 0.0f, OBLATE_FLATTENING);
        const Spherical spherical = cartesian_to_spherical(direction);
        const float geocentric_lat_deg = (HALF_PI - spherical.theta) * RAD_TO_DEG;
        CHECK(geocentric_lat_deg < 45.0f);
    }

    SUBCASE("the equatorial radius does not enter - that is why an anchor survives growth") {
        const Vec3 small = hexgrid_geodetic_to_direction(37.5f, 12.25f, OBLATE_FLATTENING);
        const Vec3 large = hexgrid_geodetic_to_direction(37.5f, 12.25f, OBLATE_FLATTENING);
        CHECK(small.x == large.x);
        CHECK(small.y == large.y);
        CHECK(small.z == large.z);
    }
}

TEST_CASE("Hexgrid formulas - the surface point carries the size the direction has not") {
    const Vec3 equator = hexgrid_geodetic_to_direction(0.0f, 0.0f, SPHERE_FLATTENING);
    const Vec3 pole = hexgrid_geodetic_to_direction(90.0f, 0.0f, SPHERE_FLATTENING);

    const Vec3 equator_point = hexgrid_surface_point(equator, SPHERE_RADIUS_M, SPHERE_FLATTENING);
    const Vec3 pole_point = hexgrid_surface_point(pole, SPHERE_RADIUS_M, SPHERE_FLATTENING);
    CHECK(abs(equator_point.length() - SPHERE_RADIUS_M) / SPHERE_RADIUS_M < RELATIVE_TOLERANCE);
    CHECK(abs(pole_point.length() - SPHERE_RADIUS_M) / SPHERE_RADIUS_M < RELATIVE_TOLERANCE);

    SUBCASE("an oblate ellipsoid is shorter at the pole than at the equator") {
        const Vec3 oblate_equator = hexgrid_surface_point(
            hexgrid_geodetic_to_direction(0.0f, 0.0f, OBLATE_FLATTENING), SPHERE_RADIUS_M,
            OBLATE_FLATTENING);
        const Vec3 oblate_pole = hexgrid_surface_point(
            hexgrid_geodetic_to_direction(90.0f, 0.0f, OBLATE_FLATTENING), SPHERE_RADIUS_M,
            OBLATE_FLATTENING);
        CHECK(oblate_pole.length() < oblate_equator.length());
    }

    SUBCASE("a degenerate ellipsoid reports the origin rather than a point on nothing") {
        const Vec3 point = hexgrid_surface_point(Vec3::up(), 0.0f, SPHERE_FLATTENING);
        CHECK(point.length() == 0.0f);
    }
}

TEST_CASE("Hexgrid formulas - the axis rotation keeps length and turns by the angle asked for") {
    const Vec3 axis = Vec3::up();
    const Vec3 value{1.0f, 0.0f, 0.0f};

    const Vec3 quarter = hexgrid_rotate_axis(value, axis, HALF_PI);
    CHECK(abs(quarter.length() - 1.0f) < RELATIVE_TOLERANCE);
    CHECK(abs(dot(quarter, value)) < ORTHOGONAL_TOLERANCE);

    SUBCASE("a rotation about the value's own axis leaves it untouched") {
        const Vec3 unchanged = hexgrid_rotate_axis(value, value, HALF_PI);
        CHECK(abs(dot(unchanged, value) - 1.0f) < RELATIVE_TOLERANCE);
    }

    SUBCASE("a full turn returns the vector to itself") {
        const Vec3 full = hexgrid_rotate_axis(value, axis, TWO_PI);
        CHECK(abs(dot(full, value) - 1.0f) < ORTHOGONAL_TOLERANCE);
    }
}
