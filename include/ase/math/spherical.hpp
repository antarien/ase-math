#pragma once

#include <ase/math/vec3.hpp>
#include <cmath>

namespace ase::math {

/**
 * Spherical coordinate conversions
 *
 * Convention:
 *   phi   = azimuthal angle (0 to 2*PI, around Y axis)
 *   theta = polar angle (0 to PI, from +Y axis)
 *   radius = distance from origin
 *
 *   Y is UP (vertical axis)
 */

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = PI / 2.0f;

/**
 * Convert spherical coordinates to cartesian (Vec3)
 *
 * @param phi Azimuthal angle in radians (0 to 2*PI, around Y axis)
 * @param theta Polar angle in radians (0 to PI, from +Y axis)
 * @param radius Distance from origin
 * @return Cartesian position (x, y, z)
 */
inline Vec3 spherical_to_cartesian(float phi, float theta, float radius) {
    const float sin_theta = std::sin(theta);
    return {
        radius * sin_theta * std::cos(phi),  // x
        radius * std::cos(theta),             // y (up)
        radius * sin_theta * std::sin(phi)   // z
    };
}

/**
 * Spherical coordinates result
 */
struct Spherical {
    float phi;      // Azimuthal angle (radians)
    float theta;    // Polar angle (radians)
    float radius;   // Distance from origin
};

/**
 * Convert cartesian coordinates to spherical
 *
 * @param v Cartesian position
 * @return Spherical coordinates {phi, theta, radius}
 */
inline Spherical cartesian_to_spherical(const Vec3& v) {
    const float radius = v.length();
    if (radius < 1e-6f) {
        return {0, 0, 0};
    }

    const float theta = std::acos(v.y / radius);  // 0 to PI
    const float phi = std::atan2(v.z, v.x);       // -PI to PI

    return {
        phi < 0 ? phi + TWO_PI : phi,  // Normalize to 0 to 2*PI
        theta,
        radius
    };
}

/**
 * Convert Right Ascension / Declination to cartesian
 * Used for celestial coordinates (stars)
 *
 * @param ra Right Ascension in hours (0 to 24)
 * @param dec Declination in degrees (-90 to 90)
 * @param radius Distance from origin
 * @return Cartesian position
 */
inline Vec3 ra_dec_to_cartesian(float ra, float dec, float radius) {
    // Convert RA hours to radians (24h = 2*PI)
    const float phi = (ra / 24.0f) * TWO_PI;

    // Convert Dec degrees to theta (polar angle from +Y)
    // Dec=90 means +Y (theta=0), Dec=-90 means -Y (theta=PI)
    const float theta = HALF_PI - (dec * PI / 180.0f);

    return spherical_to_cartesian(phi, theta, radius);
}

/**
 * Convert cartesian to Right Ascension / Declination
 *
 * @param v Cartesian position
 * @return {ra (hours), dec (degrees)}
 */
inline std::pair<float, float> cartesian_to_ra_dec(const Vec3& v) {
    const auto sph = cartesian_to_spherical(v);

    // Convert phi to RA hours
    const float ra = (sph.phi / TWO_PI) * 24.0f;

    // Convert theta to Dec degrees
    const float dec = (HALF_PI - sph.theta) * 180.0f / PI;

    return {ra, dec};
}

} // namespace ase::math
