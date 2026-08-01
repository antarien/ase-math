#pragma once

/**
 * ASE Math Planet Hex Lattice Geometry
 *
 * @file        hexgrid.hpp
 * @brief       FROZEN interface surface for the planetary hexagon lattice (SSOT)
 * @description Layer 0 foundation surface for the topology layer above the chunk grid: the geodetic
 *              anchor of a cell on the reference ellipsoid, its 5 or 6 neighbours, its edges, the
 *              pentagon predicate, the area driven cell count, and the construction chain the
 *              lattice is built from (icosahedron base, parabolic projection, convex hull, Delaunay
 *              triangulation, Voronoi dual, centroid relaxation).
 *
 *              A CELL IS A CHUNK. The address of a cell is the chunk address pair (cx,cz) as int32.
 *              There is no second cell id world and no separate cell id type - every entry point
 *              below takes and returns (cx,cz) pairs.
 *
 *              THIS SURFACE IS FROZEN (PLAN_ASE_LATTICE_PHASE_00_CONTRACT.md, WS-K.1). The
 *              signatures and constants below are the contract every lattice phase builds against.
 *              A change goes through the contract change route in PLAN_ASE_LATTICE.md, never
 *              through a silent edit.
 *
 *              LAYER 0 DOES NOT ALLOCATE. Every construction function receives caller owned buffers
 *              plus their capacity and reports how many elements it wrote. The capacity formulas
 *              are the constexpr helpers in the SIZING CONTRACT section - a caller sizes its buffer
 *              from the site count before it calls. Raw pointer spans follow the neighbouring L0
 *              convention in polygon.hpp.
 *
 *              The geometry phase (PLAN_ASE_LATTICE_PHASE_01_GEOM.md) carries the function
 *              definitions. They are header inline, following the L0 pattern of spherical.hpp,
 *              because ase-math is an INTERFACE target and owns no translation unit. Every frozen
 *              signature and every frozen constant of the contract session is unchanged; the phase
 *              added bodies and the implementation constants its construction needs, in the section
 *              marked as additive below.
 *
 * @module      ase-math
 * @layer       0 (Foundation)
 * @created     2026-07-30
 * @modified    2026-07-31
 * @version     1.1.0
 *
 * DRY / SOLID / SSOT COMPLIANCE:
 * - Cell addressing is (cx,cz):int32 everywhere - the chunk grid IS the addressing
 * - NO ECS: no registry, no EnTT, no component types in this header
 * - NO allocation: caller owned buffers plus capacity, size formulas are constexpr here
 * - NO duplicate geometry in consumer modules - all lattice math belongs HERE
 * - Spherical and cartesian conversions come from spherical.hpp, they are never restated here
 */

#include <ase/math/math.hpp>
#include <ase/math/spherical.hpp>
#include <ase/math/vec3.hpp>
#include <cstdint>
#include <cstddef>

namespace ase::math {

// =============================================================================
// TOPOLOGY CONSTANTS (design driven - DSGN_019, DSGN_017)
// =============================================================================

/**
 * A geodesic hexagon lattice over an icosahedron base carries exactly twelve pentagon cells -
 * one per icosahedron vertex. This is a property of the construction, not a tuning value.
 * Design source: DSGN_017_AWS_PROCEDURAL_GENERATION.md, Z. 101-107 (subdivided icosahedron).
 */
constexpr uint32_t HEXGRID_PENTAGON_COUNT = 12u;

/** Neighbour count of an ordinary hexagon cell (DSGN_019_AWS_AREAS_ZONES_SECTORS.md, Z. 73-76). */
constexpr uint32_t HEXGRID_MAX_NEIGHBORS = 6u;

/** Neighbour count of one of the twelve pentagon cells - five, never six. */
constexpr uint32_t HEXGRID_PENTAGON_NEIGHBORS = 5u;

/**
 * Upper bound of gates per cell: one gate per edge, six edges (DSGN_019, Z. 152-153 names a
 * maximum of six entry fronts). Gates themselves are DATA of the cell entities in the GIS phase;
 * this header only supplies the edge geometry they sit on.
 */
constexpr uint32_t HEXGRID_MAX_GATES = 6u;

/** Icosahedron base cardinalities - the seed of the whole construction. */
constexpr uint32_t HEXGRID_ICOSAHEDRON_VERTICES = 12u;
constexpr uint32_t HEXGRID_ICOSAHEDRON_EDGES = 30u;
constexpr uint32_t HEXGRID_ICOSAHEDRON_FACES = 20u;

// =============================================================================
// ENGINEERING CONSTANTS (this plan, NOT quoted from a design document)
// =============================================================================

/**
 * ENGINEERING CONSTANT of PLAN_ASE_LATTICE - it has NO design document source and must not be
 * cited as one.
 *
 * Locality bound of the centroid relaxation when the geoid radius grows. Growth adds new cells as
 * no man's land; relaxation is allowed to move only the new cells and the existing cells within
 * this ring depth around them, so established cell addresses and geodetic anchors stay stable
 * (Master DECISION D2, consequence clause).
 */
constexpr uint32_t HEXGRID_RELAX_RING_DEPTH = 2u;

/**
 * ENGINEERING CONSTANT of PLAN_ASE_LATTICE - it has NO design document source and must not be
 * cited as one.
 *
 * Relative area tolerance band of the equal distribution check: a cell passes when its area stays
 * within this fraction of the nominal cell area. 0.05 means plus or minus five percent.
 */
constexpr float HEXGRID_AREA_TOLERANCE = 0.05f;

/**
 * ENGINEERING CONSTANT of PLAN_ASE_LATTICE - it has NO design document source and must not be
 * cited as one.
 *
 * Iteration ceiling of one centroid relaxation pass. Relaxation converges asymptotically, so the
 * caller needs a hard bound to keep the pass deterministic in cost.
 */
constexpr uint32_t HEXGRID_RELAX_MAX_ITERATIONS = 32u;

// =============================================================================
// CELL AREA (DERIVED from the chunk grid - not a free design choice)
// =============================================================================

/**
 * Edge length of one cell in metres.
 *
 * DERIVED, NOT CHOSEN: the hex zone IS the chunk (Master binding decision 1), so the honeycomb
 * area is the ground area of an existing chunk. The chunk spans CHUNK_SIZE cells per axis at
 * MACRO_TILE_SIZE metres each.
 *
 * VALUE SOURCE (Layer 0 cannot include Layer 3, so this is a documented mirror, not an include):
 *   modules/ase-terrain/include/ase/terrain/types.hpp:169  CHUNK_SIZE = 32
 *   modules/ase-terrain/include/ase/terrain/types.hpp:172  MACRO_TILE_SIZE = 1.0f
 * The geometry phase pins this mirror with a test so a divergence is caught, never assumed.
 *
 * Changing this value means changing CHUNK_SIZE, which is an intervention into the terrain
 * foundation and explicitly outside this plan.
 */
constexpr float HEXGRID_CELL_EDGE_M = 32.0f;

/**
 * Nominal ground area of one cell in square metres - the A_cell of the growth formula.
 * DERIVED from HEXGRID_CELL_EDGE_M above (32 m by 32 m = 1024 square metres).
 */
constexpr float HEXGRID_CELL_AREA_M2 = HEXGRID_CELL_EDGE_M * HEXGRID_CELL_EDGE_M;

/**
 * Sentinel for a neighbour slot that carries no cell. The neighbour query reports its written
 * count, so a caller never has to compare against this value; it exists so an unwritten slot in a
 * caller buffer is unmistakable rather than a plausible looking origin cell.
 */
constexpr int32_t HEXGRID_INVALID_CELL_COORD = INT32_MIN;

// =============================================================================
// IMPLEMENTATION CONSTANTS (geometry phase, ADDITIVE)
//
// These were added by PLAN_ASE_LATTICE_PHASE_01_GEOM.md. They add to the frozen surface, they do
// not change any frozen signature or any frozen constant above. They are ENGINEERING CONSTANTS of
// that plan and have NO design document source; none of them must be cited as one.
// =============================================================================

/**
 * Epoch level ceiling of the address space.
 *
 * The lattice is a Class I subdivided icosahedron whose subdivision frequency is a power of two.
 * A growth epoch doubles the frequency, which is what makes an epoch NESTED: every site of level
 * L is also a site of level L+1, at the very same position. That nesting is the mechanism behind
 * the bit stable address and the bit stable anchor of Master DECISION D2 - the address stores the
 * position at the FINEST level, so a cell that already exists keeps its integer address and its
 * geodetic anchor across every later epoch.
 *
 * The ceiling is a precision budget, not a taste decision. The face local coordinate of an address
 * runs to HEXGRID_ADDRESS_FREQUENCY, so a barycentric weight error of e costs e times that
 * frequency in lattice steps. The projection chain below was reformulated for numerical stability
 * until its float worst case over a uniform sample of the sphere reached 3.0e-7 in barycentric
 * weight, which is 0.08 of a lattice step at level 18. Beyond level 19 that margin is spent and
 * the round trip from cell to geodetic and back would stop being an identity. At the nominal
 * HEXGRID_CELL_AREA_M2 the ceiling corresponds to an equatorial radius of about 7484 km, which is
 * above Earth size.
 */
constexpr uint32_t HEXGRID_ADDRESS_LEVEL_MAX = 18u;

/** Subdivisions per icosahedron edge at the finest level - the unit the address is stored in. */
constexpr int32_t HEXGRID_ADDRESS_FREQUENCY = static_cast<int32_t>(1u << HEXGRID_ADDRESS_LEVEL_MAX);

/**
 * Address stride between two icosahedron faces along the cx axis. Twice the frequency, so the
 * per face block [face * stride, face * stride + frequency] never touches its neighbour block.
 * The highest address this yields is 19 * 524288 + 262144, far below the int32 ceiling.
 */
constexpr int32_t HEXGRID_FACE_STRIDE = HEXGRID_ADDRESS_FREQUENCY * 2;

/**
 * A lattice site is shared by at most five icosahedron faces, which happens exactly at the twelve
 * icosahedron vertices. An edge site is shared by two, an interior site by one.
 */
constexpr uint32_t HEXGRID_MAX_CELL_REPS = 5u;

/** Angle below which a direction counts as coincident with a face centre, in radians. */
constexpr float HEXGRID_ANGLE_EPSILON = 1.0e-6f;

/** Planar radius below which a face local point counts as the face centre, in circumradius units. */
constexpr float HEXGRID_RADIUS_EPSILON = 1.0e-6f;

/** Squared length below which a difference vector counts as degenerate in the hull construction. */
constexpr float HEXGRID_HULL_EPSILON = 1.0e-12f;

/**
 * Newton steps that solve the swept area relation of the equal area projection.
 *
 * The relation is 2 sin(x/2) sin(s - x/2) = delta with a first order seed of delta / sin(s), and
 * its derivative sin(s - x) never approaches zero over the wedge, so the step count is a fixed
 * three rather than a convergence test. A fixed count is what keeps the projection deterministic
 * in both cost and result.
 */
constexpr uint32_t HEXGRID_SWEEP_NEWTON_STEPS = 3u;

/**
 * Area ratio of a pentagon cell against a hexagon cell, EXACT and structural.
 *
 * Five faces meet at an icosahedron vertex and each contributes a 60 degree sector, so the cell
 * around that vertex closes after 300 of the 360 degrees a hexagon cell spans. The equal area
 * projection carries that angular deficit unchanged into the plane, so the pentagon cell is five
 * sixths of a hexagon cell - not approximately, exactly. It is the curvature of the sphere made
 * visible, and no relaxation removes it; relaxation only spreads it over more cells.
 */
constexpr float HEXGRID_PENTAGON_AREA_RATIO = 5.0f / 6.0f;

// =============================================================================
// SIZING CONTRACT (Layer 0 does not allocate - the caller sizes from these)
// =============================================================================

/**
 * @brief Vertex capacity a convex hull over N sites needs
 * @param site_count Number of input sites
 * @return Required element count of the hull vertex buffer
 *
 * A convex hull cannot carry more vertices than it was given sites.
 */
constexpr size_t hexgrid_hull_vertex_capacity(size_t site_count) {
    return site_count;
}

/**
 * @brief Face capacity a convex hull over N sites needs
 * @param site_count Number of input sites
 * @return Required element count of the hull face buffer (triangles)
 *
 * Euler on a triangulated sphere: F equals 2N minus 4. Below four sites there is no hull.
 */
constexpr size_t hexgrid_hull_face_capacity(size_t site_count) {
    return site_count < 4 ? 0 : (2 * site_count - 4);
}

/**
 * @brief Triangle capacity the Delaunay triangulation of N sites needs
 * @param site_count Number of input sites
 * @return Required element count of the triangle buffer
 *
 * The lower hull of the parabolic lift IS the Delaunay triangulation, so it shares the hull face
 * bound (DSGN_017, Z. 107 and Z. 110).
 */
constexpr size_t hexgrid_delaunay_triangle_capacity(size_t site_count) {
    return hexgrid_hull_face_capacity(site_count);
}

/**
 * @brief Edge capacity the Delaunay triangulation of N sites needs
 * @param site_count Number of input sites
 * @return Required element count of the edge buffer
 *
 * Euler on a triangulated sphere: E equals 3N minus 6.
 */
constexpr size_t hexgrid_delaunay_edge_capacity(size_t site_count) {
    return site_count < 3 ? 0 : (3 * site_count - 6);
}

/**
 * @brief Vertex capacity the Voronoi dual of N sites needs
 * @param site_count Number of input sites
 * @return Required element count of the Voronoi vertex buffer
 *
 * One Voronoi vertex per Delaunay triangle - the dual is one to one on faces.
 */
constexpr size_t hexgrid_voronoi_vertex_capacity(size_t site_count) {
    return hexgrid_delaunay_triangle_capacity(site_count);
}

/**
 * @brief Index capacity the Voronoi cell rings of N sites need
 * @param site_count Number of input sites
 * @return Required element count of the flat ring index buffer
 *
 * Each cell ring holds at most HEXGRID_MAX_NEIGHBORS vertex indices; the rings are stored flat,
 * cell i occupying the slots [i * HEXGRID_MAX_NEIGHBORS, (i + 1) * HEXGRID_MAX_NEIGHBORS).
 */
constexpr size_t hexgrid_voronoi_index_capacity(size_t site_count) {
    return site_count * static_cast<size_t>(HEXGRID_MAX_NEIGHBORS);
}

// =============================================================================
// GEODETIC ANCHOR (cell to lat/lon on the reference ellipsoid and back)
// =============================================================================

/**
 * The reference ellipsoid is the geodetic base surface of every coordinate system
 * (DSGN_118_DIMENSIONS_SCALES_METRICS.md, section 91.3, Z. 120-122). That section carries no
 * numeric ellipsoid parameters, so the parameters are function INPUT here and are never baked in.
 *
 * semi_major_axis_m is the equatorial radius in metres; flattening is the dimensionless
 * (a - b) / a. A flattening of zero degenerates the ellipsoid into a sphere, which is legal input.
 */
struct HexgridEllipsoid {
    float semi_major_axis_m = 0.0f;  // equatorial radius a, metres
    float flattening = 0.0f;         // (a - b) / a, dimensionless, 0 means a sphere
};

// =============================================================================
// GEOMETRY CORE (geometry phase - the closed form behind every entry point below)
// =============================================================================

namespace hexgrid_detail {

/**
 * The icosahedron in its lattice orientation, plus the three wedge constants the equal area
 * projection is built from. All twelve vertices are unit vectors, Y up, matching spherical.hpp.
 */
struct HexgridSolid {
    Vec3 vertex[HEXGRID_ICOSAHEDRON_VERTICES];
    Vec3 face_center[HEXGRID_ICOSAHEDRON_FACES];
    uint32_t face_vertex[HEXGRID_ICOSAHEDRON_FACES][3];
    float center_to_vertex_rad;  // spherical distance from a face centre to a face vertex
    float vertex_wedge_rad;      // angle at a face vertex between the arc to the centre and an edge
    float wedge_area_sr;         // area of one sixth of a face, in steradian
    float sin_center_to_vertex;  // sine of center_to_vertex_rad, kept to avoid recomputing it
    float sin_vertex_wedge;      // sine of vertex_wedge_rad, kept to avoid recomputing it
    float half_center_sine_sq;   // squared sine of half center_to_vertex_rad, a stable 1 - cos form
};

/** A face local point as barycentric weights over the three corners of that face. */
struct HexgridFaceCoord {
    uint32_t face;
    float weight[3];
};

/** One representation of a lattice site: the face that carries it and its face local integers. */
struct HexgridCellRep {
    uint32_t face;
    int32_t i;
    int32_t j;
};

/** The tangent frame of one kaleidoscope wedge: face centre, direction to the corner, and lateral. */
struct HexgridWedgeFrame {
    Vec3 center;
    Vec3 radial;
    Vec3 lateral;
};

/** @brief Rodrigues rotation of a vector about a unit axis */
inline Vec3 hexgrid_rotate_axis(const Vec3& value, const Vec3& axis, float angle) {
    const float cos_a = cos(angle);
    const float sin_a = sin(angle);
    return value * cos_a + cross(axis, value) * sin_a + axis * (dot(axis, value) * (1.0f - cos_a));
}

/**
 * @brief Build the icosahedron in its lattice orientation
 * @return The solid plus its three wedge constants
 *
 * The solid is raised from spherical coordinates through spherical_to_cartesian, so the Y up
 * convention of spherical.hpp holds by construction and is never restated. The upper ring sits at
 * the polar angle acos(1/sqrt(5)), the lower ring at its supplement and offset by half a step,
 * which is the icosahedron: every listed pair is exactly one edge length apart.
 *
 * ORIENTATION SEED, DERIVED: DSGN_017 Z. 95-99 demands equally weighted, equally sized playable
 * zones and names the unplayable polar areas of a merged tiling as the defect to avoid. It does
 * NOT contain a sentence about pentagons and poles. This header derives one: the solid is tilted
 * so that the CENTRE of face 0 lands on the north pole. That is the orientation with the largest
 * possible distance between a pole and the nearest pentagon, namely center_to_vertex_rad, about
 * 37.38 degrees - no pentagon can sit on a pole, and both poles fall deep inside an ordinary
 * hexagon cell rather than on a cell boundary. The tilt is a fixed constant of this header because
 * the frozen surface carries no seed parameter; determinism therefore holds by construction.
 */
inline HexgridSolid hexgrid_build_solid() {
    HexgridSolid solid{};

    const float ring_theta = acos(1.0f / sqrt(5.0f));
    solid.vertex[0] = spherical_to_cartesian(0.0f, 0.0f, 1.0f);
    for (uint32_t k = 0u; k < 5u; ++k) {
        const float phi_upper = (static_cast<float>(k) * TWO_PI) / 5.0f;
        const float phi_lower = phi_upper + (TWO_PI / 10.0f);
        solid.vertex[1u + k] = spherical_to_cartesian(phi_upper, ring_theta, 1.0f);
        solid.vertex[6u + k] = spherical_to_cartesian(phi_lower, PI - ring_theta, 1.0f);
    }
    solid.vertex[11] = spherical_to_cartesian(0.0f, PI, 1.0f);

    uint32_t face_index = 0u;
    for (uint32_t k = 0u; k < 5u; ++k) {
        const uint32_t kn = (k + 1u) % 5u;
        solid.face_vertex[face_index][0] = 0u;
        solid.face_vertex[face_index][1] = 1u + kn;
        solid.face_vertex[face_index][2] = 1u + k;
        ++face_index;
        solid.face_vertex[face_index][0] = 1u + k;
        solid.face_vertex[face_index][1] = 1u + kn;
        solid.face_vertex[face_index][2] = 6u + k;
        ++face_index;
        solid.face_vertex[face_index][0] = 6u + k;
        solid.face_vertex[face_index][1] = 1u + kn;
        solid.face_vertex[face_index][2] = 6u + kn;
        ++face_index;
        solid.face_vertex[face_index][0] = 11u;
        solid.face_vertex[face_index][1] = 6u + k;
        solid.face_vertex[face_index][2] = 6u + kn;
        ++face_index;
    }

    const Vec3 seed_center = normalize(solid.vertex[solid.face_vertex[0][0]] +
                                       solid.vertex[solid.face_vertex[0][1]] +
                                       solid.vertex[solid.face_vertex[0][2]]);
    const Vec3 pole = Vec3::up();
    const Vec3 tilt_axis = normalize(cross(seed_center, pole));
    const float tilt_angle = acos(clamp(dot(seed_center, pole), -1.0f, 1.0f));
    for (uint32_t vi = 0u; vi < HEXGRID_ICOSAHEDRON_VERTICES; ++vi) {
        solid.vertex[vi] = normalize(hexgrid_rotate_axis(solid.vertex[vi], tilt_axis, tilt_angle));
    }
    for (uint32_t f = 0u; f < HEXGRID_ICOSAHEDRON_FACES; ++f) {
        solid.face_center[f] = normalize(solid.vertex[solid.face_vertex[f][0]] +
                                         solid.vertex[solid.face_vertex[f][1]] +
                                         solid.vertex[solid.face_vertex[f][2]]);
    }

    const Vec3 corner = solid.vertex[solid.face_vertex[0][0]];
    const Vec3 next_corner = solid.vertex[solid.face_vertex[0][1]];
    const Vec3 center = solid.face_center[0];
    solid.center_to_vertex_rad = acos(clamp(dot(center, corner), -1.0f, 1.0f));
    const Vec3 to_center = normalize(center - corner * dot(corner, center));
    const Vec3 to_next = normalize(next_corner - corner * dot(corner, next_corner));
    solid.vertex_wedge_rad = acos(clamp(dot(to_center, to_next), -1.0f, 1.0f));
    solid.wedge_area_sr = solid.vertex_wedge_rad - (PI / 6.0f);
    solid.sin_center_to_vertex = sin(solid.center_to_vertex_rad);
    solid.sin_vertex_wedge = sin(solid.vertex_wedge_rad);
    const float half_center_sine = sin(solid.center_to_vertex_rad * 0.5f);
    solid.half_center_sine_sq = half_center_sine * half_center_sine;
    return solid;
}

/** @brief The one lattice solid, built once and shared by every query */
inline const HexgridSolid& hexgrid_solid() {
    static const HexgridSolid solid = hexgrid_build_solid();
    return solid;
}

/** @brief Tangent frame of the wedge that belongs to one corner of one face */
inline HexgridWedgeFrame hexgrid_wedge_frame(const HexgridSolid& solid, uint32_t face,
                                             uint32_t corner) {
    HexgridWedgeFrame frame{};
    frame.center = solid.face_center[face];
    const Vec3 corner_dir = solid.vertex[solid.face_vertex[face][corner]];
    frame.radial = normalize(corner_dir - frame.center * dot(frame.center, corner_dir));
    frame.lateral = normalize(cross(frame.center, frame.radial));
    return frame;
}

/**
 * @brief Equal area projection of a sphere direction into face local barycentric weights
 * @param direction Unit direction on the sphere
 * @return The face that contains it plus the barycentric weights over that face
 *
 * WHY EQUAL AREA AND NOT THE OBVIOUS GNOMONIC MAP: a plain barycentric subdivision that is
 * normalised onto the sphere stretches the cells near a face vertex against those near a face
 * centre by the cube of a secant, which is a factor of about two. Cells would then be equal only
 * in name, and DSGN_017 Z. 95-99 asks for equally sized playable zones, not for equally counted
 * ones. The map below is area preserving by construction, so a regular triangular site lattice in
 * the plane becomes a honeycomb of exactly equal area on the sphere.
 *
 * THE CONSTRUCTION, derived here and not quoted: a face is cut into six congruent wedges by its
 * three corners and three edge midpoints. Inside one wedge a direction is described by the polar
 * angle z away from the face centre and the azimuth az away from the corner. The spherical area
 * swept between azimuth zero and az is the area of the spherical triangle formed by the face
 * centre, the corner and the point where the azimuth ray meets the face edge. With the side
 * center_to_vertex_rad between the two known angles az and vertex_wedge_rad, the third angle
 * follows from the spherical law of cosines for angles, so that swept area is closed form. The
 * planar wedge is the matching sixth of an equilateral triangle, whose swept area is a tangent.
 * Matching the two sweeps fixes the azimuth correspondence; matching the radial sweeps fixes the
 * radius. The direction from the plane back to the sphere is fully closed form, which matters
 * because that is the direction every cell anchor takes; this direction solves one scalar relation
 * with a fixed three step Newton, because the closed form for it subtracts two quantities of size
 * pi and would throw away most of a float mantissa doing so.
 * Cross check of the derivation: it reproduces the two classical constants of an equal
 * area icosahedral projection, 37.37736814 degrees for center_to_vertex_rad and exactly 36 degrees
 * for vertex_wedge_rad, and it yields exactly pi/30 steradian per wedge, which times 120 wedges is
 * the full sphere.
 */
inline HexgridFaceCoord hexgrid_direction_to_face_coord(const Vec3& direction) {
    const HexgridSolid& solid = hexgrid_solid();

    uint32_t face = 0u;
    float best_center = -2.0f;
    for (uint32_t f = 0u; f < HEXGRID_ICOSAHEDRON_FACES; ++f) {
        const float aligned = dot(direction, solid.face_center[f]);
        if (aligned > best_center) {
            best_center = aligned;
            face = f;
        }
    }

    HexgridFaceCoord coord{};
    coord.face = face;

    const Vec3 center = solid.face_center[face];

    // The polar span enters only as 1 minus its cosine, and near the face centre that subtraction
    // would cancel away most of the mantissa. The chord carries the same quantity without any
    // cancellation, because half the squared chord length IS 1 minus the cosine.
    const Vec3 chord = direction - center;
    const float polar_span = 0.5f * dot(chord, chord);
    const Vec3 perpendicular = chord - center * dot(chord, center);
    if (dot(perpendicular, perpendicular) < HEXGRID_ANGLE_EPSILON * HEXGRID_ANGLE_EPSILON) {
        coord.weight[0] = 1.0f / 3.0f;
        coord.weight[1] = 1.0f / 3.0f;
        coord.weight[2] = 1.0f / 3.0f;
        return coord;
    }

    const Vec3 tangent = normalize(perpendicular);
    uint32_t corner = 0u;
    float signed_azimuth = 0.0f;
    float smallest = 10.0f;
    for (uint32_t c = 0u; c < 3u; ++c) {
        const HexgridWedgeFrame frame = hexgrid_wedge_frame(solid, face, c);
        const float candidate = atan2(dot(tangent, frame.lateral), dot(tangent, frame.radial));
        if (abs(candidate) < smallest) {
            smallest = abs(candidate);
            corner = c;
            signed_azimuth = candidate;
        }
    }

    const float azimuth = abs(signed_azimuth);
    const float side = (signed_azimuth < 0.0f) ? -1.0f : 1.0f;
    const float wedge_angle = solid.vertex_wedge_rad;
    const float outer_angle = azimuth + wedge_angle;

    // Swept area of the wedge between the corner ray and this azimuth. Writing it as the sum of
    // three angles minus pi would subtract two quantities of size pi to get one of size 0.05, so
    // the equivalent implicit relation is solved instead: it holds the same root and every term in
    // it stays at its own scale.
    const float sweep_target =
        2.0f * sin(azimuth) * solid.sin_vertex_wedge * solid.half_center_sine_sq;
    float swept = sweep_target / sin(outer_angle);
    for (uint32_t step = 0u; step < HEXGRID_SWEEP_NEWTON_STEPS; ++step) {
        const float residual =
            sweep_target - 2.0f * sin(swept * 0.5f) * sin(outer_angle - swept * 0.5f);
        swept += residual / sin(outer_angle - swept);
    }

    const float swept_fraction = clamp(swept / solid.wedge_area_sr, 0.0f, 1.0f);
    const float third_pi = PI / 3.0f;
    const float root_three = sqrt(3.0f);
    const float planar_angle = third_pi + atan2(root_three * (swept_fraction - 1.0f), 1.0f);
    const float sin_edge = clamp(solid.sin_center_to_vertex * solid.sin_vertex_wedge /
                                     sin(outer_angle - swept),
                                 -1.0f, 1.0f);
    const float cos_edge = sqrt(max(0.0f, 1.0f - sin_edge * sin_edge));
    const float edge_span = max((sin_edge * sin_edge) / (1.0f + cos_edge), HEXGRID_ANGLE_EPSILON);
    const float edge_radius = 0.5f / cos(planar_angle - third_pi);
    const float radius = edge_radius * sqrt(max(0.0f, polar_span / edge_span));
    const float planar_signed = side * planar_angle;
    const float two_third_pi = TWO_PI / 3.0f;

    coord.weight[corner] = (1.0f + 2.0f * radius * cos(planar_signed)) / 3.0f;
    coord.weight[(corner + 1u) % 3u] = (1.0f + 2.0f * radius * cos(planar_signed - two_third_pi)) / 3.0f;
    coord.weight[(corner + 2u) % 3u] = (1.0f + 2.0f * radius * cos(planar_signed + two_third_pi)) / 3.0f;

    float total = 0.0f;
    for (uint32_t c = 0u; c < 3u; ++c) {
        coord.weight[c] = max(0.0f, coord.weight[c]);
        total += coord.weight[c];
    }
    if (total > 0.0f) {
        for (uint32_t c = 0u; c < 3u; ++c) {
            coord.weight[c] /= total;
        }
    }
    return coord;
}

/**
 * @brief Inverse of hexgrid_direction_to_face_coord
 * @param face Icosahedron face index
 * @param weight Barycentric weights over that face, non negative and summing to one
 * @return Unit direction on the sphere, Y up
 *
 * Same wedge, walked backwards. The azimuth is recovered in closed form: writing the swept area as
 * the sum of the azimuth and the far angle turns the spherical law of cosines for angles into a
 * linear combination of the sine and the cosine of the azimuth, and an atan2 solves it exactly.
 */
inline Vec3 hexgrid_face_coord_to_direction(uint32_t face, const float weight[3]) {
    const HexgridSolid& solid = hexgrid_solid();

    uint32_t corner = 0u;
    for (uint32_t c = 1u; c < 3u; ++c) {
        if (weight[c] > weight[corner]) {
            corner = c;
        }
    }
    const HexgridWedgeFrame frame = hexgrid_wedge_frame(solid, face, corner);

    const float root_three = sqrt(3.0f);
    const float own = 3.0f * weight[corner] - 1.0f;
    const float next = 3.0f * weight[(corner + 1u) % 3u] - 1.0f;
    const float planar_x = own;
    const float planar_y = (own + 2.0f * next) / root_three;
    const float radius = sqrt(planar_x * planar_x + planar_y * planar_y) * 0.5f;
    if (radius < HEXGRID_RADIUS_EPSILON) {
        return frame.center;
    }

    const float planar_signed = atan2(planar_y, planar_x);
    const float planar_angle = abs(planar_signed);
    const float side = (planar_signed < 0.0f) ? -1.0f : 1.0f;
    const float third_pi = PI / 3.0f;
    const float wedge_angle = solid.vertex_wedge_rad;
    const float edge_radius = 0.5f / cos(planar_angle - third_pi);
    const float radial_ratio = (radius / edge_radius) * (radius / edge_radius);
    const float swept_fraction = 1.0f + (tan(planar_angle - third_pi) / root_three);
    const float swept = swept_fraction * solid.wedge_area_sr;

    // Azimuth from the swept area, in closed form. Both components come from the sum to product
    // identities rather than from a direct difference of two cosines of similar size, which is
    // what keeps the small result at full precision instead of at the precision of the operands.
    const float half_swept = swept * 0.5f;
    const float numerator = 2.0f * sin(half_swept) * sin(wedge_angle - half_swept);
    const float denominator = 2.0f * solid.sin_vertex_wedge * solid.half_center_sine_sq -
                              2.0f * cos(wedge_angle - half_swept) * sin(half_swept);
    const float azimuth = atan2(numerator, denominator);
    const float sin_edge = clamp(solid.sin_center_to_vertex * solid.sin_vertex_wedge /
                                     sin(azimuth + wedge_angle - swept),
                                 -1.0f, 1.0f);
    const float cos_edge = sqrt(max(0.0f, 1.0f - sin_edge * sin_edge));
    const float edge_span = (sin_edge * sin_edge) / (1.0f + cos_edge);
    const float polar_span = clamp(radial_ratio * edge_span, 0.0f, 2.0f);
    const float cos_z = 1.0f - polar_span;
    const float sin_z = sqrt(max(0.0f, polar_span * (2.0f - polar_span)));
    const Vec3 tangent = frame.radial * cos(azimuth) + frame.lateral * (side * sin(azimuth));
    return normalize(frame.center * cos_z + tangent * sin_z);
}

/**
 * @brief Split a cell address into face and face local integers
 * @return false when the pair addresses no site of the lattice
 */
inline bool hexgrid_decode_cell(int32_t cx, int32_t cz, uint32_t& face, int32_t& i, int32_t& j) {
    if (cx < 0 || cz < 0) {
        return false;
    }
    const int32_t face_index = cx / HEXGRID_FACE_STRIDE;
    if (face_index >= static_cast<int32_t>(HEXGRID_ICOSAHEDRON_FACES)) {
        return false;
    }
    const int32_t local_i = cx - face_index * HEXGRID_FACE_STRIDE;
    if (local_i > HEXGRID_ADDRESS_FREQUENCY || cz > HEXGRID_ADDRESS_FREQUENCY) {
        return false;
    }
    if (local_i + cz > HEXGRID_ADDRESS_FREQUENCY) {
        return false;
    }
    face = static_cast<uint32_t>(face_index);
    i = local_i;
    j = cz;
    return true;
}

/**
 * @brief Every face that carries this site, in ascending face order
 * @return Number of representations written, one for an interior site, two on a face edge, five at
 *         an icosahedron vertex
 *
 * A site is identified by which icosahedron vertices carry a non zero barycentric weight. Any face
 * whose corners cover that carrier set holds the same point, and its own face local integers fall
 * out of the weights. The lowest face index is the canonical one, which is what makes the address
 * unique. No adjacency table is needed and none is kept.
 */
inline uint32_t hexgrid_cell_reps(uint32_t face, int32_t i, int32_t j, HexgridCellRep* out,
                                  uint32_t capacity) {
    const HexgridSolid& solid = hexgrid_solid();
    const int32_t weight[3] = {HEXGRID_ADDRESS_FREQUENCY - i - j, i, j};
    const uint32_t carrier[3] = {solid.face_vertex[face][0], solid.face_vertex[face][1],
                                 solid.face_vertex[face][2]};

    uint32_t written = 0u;
    for (uint32_t f = 0u; f < HEXGRID_ICOSAHEDRON_FACES; ++f) {
        int32_t mapped[3] = {0, 0, 0};
        bool covered = true;
        for (uint32_t c = 0u; c < 3u; ++c) {
            if (weight[c] == 0) {
                continue;
            }
            bool found = false;
            for (uint32_t d = 0u; d < 3u; ++d) {
                if (solid.face_vertex[f][d] == carrier[c]) {
                    mapped[d] += weight[c];
                    found = true;
                    break;
                }
            }
            if (!found) {
                covered = false;
                break;
            }
        }
        if (!covered) {
            continue;
        }
        if (written < capacity) {
            out[written].face = f;
            out[written].i = mapped[1];
            out[written].j = mapped[2];
        }
        ++written;
    }
    return written;
}

/** @brief Canonical address of a face local site, the representation on the lowest face index */
inline bool hexgrid_canonical_cell(uint32_t face, int32_t i, int32_t j, int32_t& cx, int32_t& cz) {
    HexgridCellRep reps[HEXGRID_MAX_CELL_REPS];
    const uint32_t written = hexgrid_cell_reps(face, i, j, reps, HEXGRID_MAX_CELL_REPS);
    if (written == 0u) {
        return false;
    }
    cx = static_cast<int32_t>(reps[0].face) * HEXGRID_FACE_STRIDE + reps[0].i;
    cz = reps[0].j;
    return true;
}

/** @brief Surface area of the reference ellipsoid in square metres */
inline float hexgrid_surface_area(const HexgridEllipsoid& ellipsoid) {
    const float semi_major = ellipsoid.semi_major_axis_m;
    if (semi_major <= 0.0f) {
        return 0.0f;
    }
    const float flattening = clamp(ellipsoid.flattening, 0.0f, 0.999f);
    const float eccentricity_sq = 2.0f * flattening - flattening * flattening;
    if (eccentricity_sq < 1.0e-9f) {
        return 4.0f * PI * semi_major * semi_major;
    }
    const float eccentricity = sqrt(eccentricity_sq);
    const float area_tanh = 0.5f * log((1.0f + eccentricity) / (1.0f - eccentricity));
    return 2.0f * PI * semi_major * semi_major *
           (1.0f + ((1.0f - eccentricity_sq) / eccentricity) * area_tanh);
}

/**
 * @brief Epoch level of a given ellipsoid at a given honeycomb area
 *
 * The lattice frequency is the largest power of two whose site count still fits into the surface
 * at the requested honeycomb area. Growth epochs are exactly the steps of this level, which is
 * what Master DECISION D2 calls an epoch: between two epochs nothing about the lattice moves.
 */
inline uint32_t hexgrid_epoch_level(const HexgridEllipsoid& ellipsoid, float cell_area_m2) {
    if (cell_area_m2 <= 0.0f) {
        return 0u;
    }
    const float area = hexgrid_surface_area(ellipsoid);
    uint32_t level = 0u;
    while (level < HEXGRID_ADDRESS_LEVEL_MAX) {
        const float frequency = static_cast<float>(1u << (level + 1u));
        if (10.0f * frequency * frequency * cell_area_m2 > area) {
            break;
        }
        ++level;
    }
    return level;
}

/** @brief Address stride between two live sites at a given epoch level */
inline int32_t hexgrid_epoch_step(uint32_t level) {
    const uint32_t clamped = min(level, HEXGRID_ADDRESS_LEVEL_MAX);
    return static_cast<int32_t>(1u << (HEXGRID_ADDRESS_LEVEL_MAX - clamped));
}

/** @brief Does this face local site exist at the given epoch step */
inline bool hexgrid_is_live_site(int32_t i, int32_t j, int32_t step) {
    return (i % step) == 0 && (j % step) == 0;
}

/** @brief Direction of a face local integer site */
inline Vec3 hexgrid_site_direction(uint32_t face, int32_t i, int32_t j) {
    const float frequency = static_cast<float>(HEXGRID_ADDRESS_FREQUENCY);
    float weight[3];
    weight[1] = static_cast<float>(i) / frequency;
    weight[2] = static_cast<float>(j) / frequency;
    weight[0] = 1.0f - weight[1] - weight[2];
    return hexgrid_face_coord_to_direction(face, weight);
}

/** @brief Geodetic latitude and longitude of a geocentric unit direction */
inline void hexgrid_direction_to_geodetic(const Vec3& direction, const HexgridEllipsoid& ellipsoid,
                                          float& latitude_deg, float& longitude_deg) {
    const Spherical spherical = cartesian_to_spherical(direction);
    const float geocentric_lat = HALF_PI - spherical.theta;
    const float polar_ratio = 1.0f - clamp(ellipsoid.flattening, 0.0f, 0.999f);
    const float squashed = polar_ratio * polar_ratio;
    const float geodetic_lat = atan2(sin(geocentric_lat), squashed * cos(geocentric_lat));
    latitude_deg = geodetic_lat * RAD_TO_DEG;
    float longitude = spherical.phi;
    if (longitude > PI) {
        longitude -= TWO_PI;
    }
    longitude_deg = longitude * RAD_TO_DEG;
}

/** @brief Geocentric unit direction of a geodetic latitude and longitude */
inline Vec3 hexgrid_geodetic_to_direction(float latitude_deg, float longitude_deg,
                                          const HexgridEllipsoid& ellipsoid) {
    const float geodetic_lat = clamp(latitude_deg, -90.0f, 90.0f) * DEG_TO_RAD;
    const float polar_ratio = 1.0f - clamp(ellipsoid.flattening, 0.0f, 0.999f);
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

/** @brief Point where a geocentric direction pierces the ellipsoid surface, Y up */
inline Vec3 hexgrid_surface_point(const Vec3& direction, const HexgridEllipsoid& ellipsoid) {
    const Spherical spherical = cartesian_to_spherical(direction);
    const float semi_major = ellipsoid.semi_major_axis_m;
    const float semi_minor = semi_major * (1.0f - clamp(ellipsoid.flattening, 0.0f, 0.999f));
    if (semi_major <= 0.0f || semi_minor <= 0.0f) {
        return Vec3::zero();
    }
    const float sin_theta = sin(spherical.theta);
    const float cos_theta = cos(spherical.theta);
    const float inverse = sqrt((sin_theta * sin_theta) / (semi_major * semi_major) +
                               (cos_theta * cos_theta) / (semi_minor * semi_minor));
    const float radius = (inverse > 0.0f) ? (1.0f / inverse) : 0.0f;
    return spherical_to_cartesian(spherical.phi, spherical.theta, radius);
}

/**
 * @brief Nearest live site of the epoch lattice to a face local barycentric point
 *
 * Rounding in a triangular lattice is not three independent roundings: the three scaled weights
 * must stay an exact sum, so the component that was rounded hardest is corrected. That is the
 * classical cube rounding rule and it lands on the nearest site, never on a diagonal neighbour.
 */
inline void hexgrid_round_to_site(const HexgridFaceCoord& coord, int32_t step, int32_t& i,
                                  int32_t& j) {
    const int32_t frequency = HEXGRID_ADDRESS_FREQUENCY / step;
    const float scale = static_cast<float>(frequency);
    const float exact[3] = {coord.weight[0] * scale, coord.weight[1] * scale,
                            coord.weight[2] * scale};
    int32_t rounded[3];
    float drift[3];
    for (uint32_t c = 0u; c < 3u; ++c) {
        rounded[c] = static_cast<int32_t>(round(exact[c]));
        if (rounded[c] < 0) {
            rounded[c] = 0;
        }
        drift[c] = abs(static_cast<float>(rounded[c]) - exact[c]);
    }
    const int32_t sum = rounded[0] + rounded[1] + rounded[2];
    if (sum != frequency) {
        uint32_t worst = 0u;
        for (uint32_t c = 1u; c < 3u; ++c) {
            if (drift[c] > drift[worst]) {
                worst = c;
            }
        }
        int32_t others = 0;
        for (uint32_t c = 0u; c < 3u; ++c) {
            if (c != worst) {
                others += rounded[c];
            }
        }
        rounded[worst] = frequency - others;
        if (rounded[worst] < 0) {
            rounded[worst] = 0;
        }
    }
    i = rounded[1] * step;
    j = rounded[2] * step;
    if (i + j > HEXGRID_ADDRESS_FREQUENCY) {
        j = HEXGRID_ADDRESS_FREQUENCY - i;
    }
}

/** @brief Insertion sort of a small index set by an accompanying key, ascending */
inline void hexgrid_sort_by_key(float* key, uint32_t* order, uint32_t count) {
    for (uint32_t a = 1u; a < count; ++a) {
        const float pivot_key = key[a];
        const uint32_t pivot_order = order[a];
        uint32_t b = a;
        while (b > 0u && key[b - 1u] > pivot_key) {
            key[b] = key[b - 1u];
            order[b] = order[b - 1u];
            --b;
        }
        key[b] = pivot_key;
        order[b] = pivot_order;
    }
}

/**
 * The full ring of one cell: its neighbours and its cell corners, both in counter clockwise order
 * and interleaved, so that neighbour k is bounded by corner k-1 and corner k.
 */
struct HexgridCellRing {
    uint32_t count;
    int32_t neighbor_cx[HEXGRID_MAX_NEIGHBORS];
    int32_t neighbor_cz[HEXGRID_MAX_NEIGHBORS];
    Vec3 corner[HEXGRID_MAX_NEIGHBORS];
    Vec3 center;
    bool valid;
};

/**
 * @brief Neighbours and corners of one cell at one epoch
 *
 * Both sets are collected over every face that carries the cell, which is how a face boundary is
 * crossed without a single unfolding formula: the piece of the ring that leaves one face is simply
 * the piece that another face already holds. An interior cell contributes six of each from one
 * face, a face edge cell three from each of two faces, and an icosahedron vertex cell exactly one
 * corner and two shared neighbours from each of its five faces, which is where the five falls out
 * of the geometry rather than out of a special case. A cell corner is the centroid of a lattice
 * triangle and therefore sits at a third of a step, so it never lands on a face edge and is always
 * carried by exactly one face - which is why the corner set needs no de-duplication.
 */
inline HexgridCellRing hexgrid_cell_ring(int32_t cx, int32_t cz, uint32_t level) {
    HexgridCellRing ring{};
    ring.valid = false;

    uint32_t face = 0u;
    int32_t i = 0;
    int32_t j = 0;
    if (!hexgrid_decode_cell(cx, cz, face, i, j)) {
        return ring;
    }
    const int32_t step = hexgrid_epoch_step(level);
    if (!hexgrid_is_live_site(i, j, step)) {
        return ring;
    }

    HexgridCellRep reps[HEXGRID_MAX_CELL_REPS];
    const uint32_t rep_count = min(hexgrid_cell_reps(face, i, j, reps, HEXGRID_MAX_CELL_REPS),
                                   HEXGRID_MAX_CELL_REPS);
    if (rep_count == 0u) {
        return ring;
    }

    ring.center = hexgrid_site_direction(reps[0].face, reps[0].i, reps[0].j);

    const HexgridSolid& solid = hexgrid_solid();
    const Vec3 face_center = solid.face_center[reps[0].face];
    const float aligned = dot(ring.center, face_center);
    const Vec3 reference = normalize(face_center - ring.center * aligned);
    const Vec3 lateral = cross(ring.center, reference);

    const int32_t neighbor_offset[HEXGRID_MAX_NEIGHBORS][2] = {{1, 0},  {0, 1},  {-1, 1},
                                                               {-1, 0}, {0, -1}, {1, -1}};
    const int32_t corner_offset[HEXGRID_MAX_NEIGHBORS][2] = {{1, 1},   {-1, 2}, {-2, 1},
                                                             {-1, -1}, {1, -2}, {2, -1}};

    int32_t collected_cx[HEXGRID_MAX_NEIGHBORS];
    int32_t collected_cz[HEXGRID_MAX_NEIGHBORS];
    float neighbor_key[HEXGRID_MAX_NEIGHBORS];
    uint32_t neighbor_order[HEXGRID_MAX_NEIGHBORS];
    uint32_t neighbor_count = 0u;

    Vec3 collected_corner[HEXGRID_MAX_NEIGHBORS];
    float corner_key[HEXGRID_MAX_NEIGHBORS];
    uint32_t corner_order[HEXGRID_MAX_NEIGHBORS];
    uint32_t corner_count = 0u;

    for (uint32_t r = 0u; r < rep_count; ++r) {
        for (uint32_t k = 0u; k < HEXGRID_MAX_NEIGHBORS; ++k) {
            const int32_t ni = reps[r].i + neighbor_offset[k][0] * step;
            const int32_t nj = reps[r].j + neighbor_offset[k][1] * step;
            if (ni < 0 || nj < 0 || ni + nj > HEXGRID_ADDRESS_FREQUENCY) {
                continue;
            }
            int32_t ncx = 0;
            int32_t ncz = 0;
            if (!hexgrid_canonical_cell(reps[r].face, ni, nj, ncx, ncz)) {
                continue;
            }
            bool seen = false;
            for (uint32_t t = 0u; t < neighbor_count; ++t) {
                if (collected_cx[t] == ncx && collected_cz[t] == ncz) {
                    seen = true;
                    break;
                }
            }
            if (seen || neighbor_count >= HEXGRID_MAX_NEIGHBORS) {
                continue;
            }
            const Vec3 direction = hexgrid_site_direction(reps[r].face, ni, nj);
            const Vec3 offset = direction - ring.center * dot(direction, ring.center);
            collected_cx[neighbor_count] = ncx;
            collected_cz[neighbor_count] = ncz;
            neighbor_key[neighbor_count] = atan2(dot(offset, lateral), dot(offset, reference));
            neighbor_order[neighbor_count] = neighbor_count;
            ++neighbor_count;
        }

        for (uint32_t k = 0u; k < HEXGRID_MAX_NEIGHBORS; ++k) {
            const float third = static_cast<float>(step) / 3.0f;
            const float ci = static_cast<float>(reps[r].i) +
                             static_cast<float>(corner_offset[k][0]) * third;
            const float cj = static_cast<float>(reps[r].j) +
                             static_cast<float>(corner_offset[k][1]) * third;
            const float frequency = static_cast<float>(HEXGRID_ADDRESS_FREQUENCY);
            float weight[3];
            weight[1] = ci / frequency;
            weight[2] = cj / frequency;
            weight[0] = 1.0f - weight[1] - weight[2];
            if (weight[0] <= 0.0f || weight[1] <= 0.0f || weight[2] <= 0.0f) {
                continue;
            }
            if (corner_count >= HEXGRID_MAX_NEIGHBORS) {
                continue;
            }
            const Vec3 direction = hexgrid_face_coord_to_direction(reps[r].face, weight);
            const Vec3 offset = direction - ring.center * dot(direction, ring.center);
            collected_corner[corner_count] = direction;
            corner_key[corner_count] = atan2(dot(offset, lateral), dot(offset, reference));
            corner_order[corner_count] = corner_count;
            ++corner_count;
        }
    }

    if (neighbor_count == 0u || neighbor_count != corner_count) {
        return ring;
    }

    hexgrid_sort_by_key(neighbor_key, neighbor_order, neighbor_count);
    hexgrid_sort_by_key(corner_key, corner_order, corner_count);

    for (uint32_t k = 0u; k < neighbor_count; ++k) {
        ring.neighbor_cx[k] = collected_cx[neighbor_order[k]];
        ring.neighbor_cz[k] = collected_cz[neighbor_order[k]];
    }

    // Rotate the corner ring so that corner k is the one that follows neighbour k in azimuth.
    uint32_t lead = 0u;
    for (uint32_t k = 0u; k < corner_count; ++k) {
        if (corner_key[k] > neighbor_key[0]) {
            lead = k;
            break;
        }
        lead = corner_count;
    }
    if (lead >= corner_count) {
        lead = 0u;
    }
    for (uint32_t k = 0u; k < corner_count; ++k) {
        ring.corner[k] = collected_corner[corner_order[(lead + k) % corner_count]];
    }

    ring.count = neighbor_count;
    ring.valid = true;
    return ring;
}

}  // namespace hexgrid_detail

/**
 * @brief Geodetic anchor of a cell on the reference ellipsoid
 * @param cx Cell chunk X address
 * @param cz Cell chunk Z address
 * @param ellipsoid Reference ellipsoid parameters
 * @param[out] latitude_deg Geodetic latitude in degrees, -90 to 90
 * @param[out] longitude_deg Geodetic longitude in degrees, -180 to 180
 *
 * The anchor is the cell centre. Stable across geoid growth: a cell that exists keeps its anchor
 * when the radius grows (Master binding decision 3). That stability is structural, not promised -
 * the anchor is a function of the face local address alone, and semi_major_axis_m does not enter
 * the computation at all. Only the flattening does, because it separates geodetic from geocentric
 * latitude. An address outside the addressable set has no anchor and reports zero for both.
 */
inline void hexgrid_cell_to_geodetic(int32_t cx, int32_t cz, const HexgridEllipsoid& ellipsoid,
                                     float& latitude_deg, float& longitude_deg) {
    uint32_t face = 0u;
    int32_t i = 0;
    int32_t j = 0;
    if (!hexgrid_detail::hexgrid_decode_cell(cx, cz, face, i, j)) {
        latitude_deg = 0.0f;
        longitude_deg = 0.0f;
        return;
    }
    const Vec3 direction = hexgrid_detail::hexgrid_site_direction(face, i, j);
    hexgrid_detail::hexgrid_direction_to_geodetic(direction, ellipsoid, latitude_deg, longitude_deg);
}

/**
 * @brief Cell that contains a geodetic position
 * @param latitude_deg Geodetic latitude in degrees, -90 to 90
 * @param longitude_deg Geodetic longitude in degrees, -180 to 180
 * @param ellipsoid Reference ellipsoid parameters
 * @param[out] cx Cell chunk X address
 * @param[out] cz Cell chunk Z address
 *
 * Inverse of hexgrid_cell_to_geodetic. Exactly one cell contains any given position: the position
 * is projected into its face, rounded onto the live lattice of the epoch the ellipsoid implies,
 * and reduced to the canonical face. Poles and pentagons take the same path as any other position;
 * there is no branch for them and no band the mapping is restricted to.
 */
inline void hexgrid_geodetic_to_cell(float latitude_deg, float longitude_deg,
                                     const HexgridEllipsoid& ellipsoid, int32_t& cx, int32_t& cz) {
    const Vec3 direction =
        hexgrid_detail::hexgrid_geodetic_to_direction(latitude_deg, longitude_deg, ellipsoid);
    const hexgrid_detail::HexgridFaceCoord coord =
        hexgrid_detail::hexgrid_direction_to_face_coord(direction);
    const uint32_t level = hexgrid_detail::hexgrid_epoch_level(ellipsoid, HEXGRID_CELL_AREA_M2);
    const int32_t step = hexgrid_detail::hexgrid_epoch_step(level);
    int32_t i = 0;
    int32_t j = 0;
    hexgrid_detail::hexgrid_round_to_site(coord, step, i, j);
    if (!hexgrid_detail::hexgrid_canonical_cell(coord.face, i, j, cx, cz)) {
        cx = HEXGRID_INVALID_CELL_COORD;
        cz = HEXGRID_INVALID_CELL_COORD;
    }
}

/**
 * @brief Cartesian position of a cell centre on the reference ellipsoid
 * @param cx Cell chunk X address
 * @param cz Cell chunk Z address
 * @param ellipsoid Reference ellipsoid parameters
 * @return Cartesian position, Y up, matching the spherical.hpp convention
 */
inline Vec3 hexgrid_cell_to_cartesian(int32_t cx, int32_t cz, const HexgridEllipsoid& ellipsoid) {
    uint32_t face = 0u;
    int32_t i = 0;
    int32_t j = 0;
    if (!hexgrid_detail::hexgrid_decode_cell(cx, cz, face, i, j)) {
        return Vec3::zero();
    }
    const Vec3 direction = hexgrid_detail::hexgrid_site_direction(face, i, j);
    return hexgrid_detail::hexgrid_surface_point(direction, ellipsoid);
}

// =============================================================================
// NEIGHBOURHOOD, EDGES, PENTAGON PREDICATE
// =============================================================================

/**
 * @brief Is this cell one of the twelve pentagon cells
 * @param cx Cell chunk X address
 * @param cz Cell chunk Z address
 * @param ellipsoid Reference ellipsoid parameters
 * @return true for exactly HEXGRID_PENTAGON_COUNT cells of the lattice
 *
 * Deterministic from the icosahedron base: the pentagons sit at the twelve icosahedron vertices.
 * Pentagons are DATA, never a special case branch in a consumer - a consumer reads the neighbour
 * count and iterates it. An icosahedron vertex is a site of every epoch level, so the twelve stay
 * twelve while the planet grows.
 */
inline bool hexgrid_cell_is_pentagon(int32_t cx, int32_t cz, const HexgridEllipsoid& ellipsoid) {
    uint32_t face = 0u;
    int32_t i = 0;
    int32_t j = 0;
    if (!hexgrid_detail::hexgrid_decode_cell(cx, cz, face, i, j)) {
        return false;
    }
    const uint32_t level = hexgrid_detail::hexgrid_epoch_level(ellipsoid, HEXGRID_CELL_AREA_M2);
    if (!hexgrid_detail::hexgrid_is_live_site(i, j, hexgrid_detail::hexgrid_epoch_step(level))) {
        return false;
    }
    const int32_t frequency = HEXGRID_ADDRESS_FREQUENCY;
    return (i == 0 && j == 0) || (i == frequency && j == 0) || (i == 0 && j == frequency);
}

/**
 * @brief Number of neighbours this cell has
 * @param cx Cell chunk X address
 * @param cz Cell chunk Z address
 * @param ellipsoid Reference ellipsoid parameters
 * @return HEXGRID_PENTAGON_NEIGHBORS for a pentagon cell, HEXGRID_MAX_NEIGHBORS otherwise, and
 *         zero for an address that is no cell of this epoch
 */
inline uint32_t hexgrid_cell_neighbor_count(int32_t cx, int32_t cz,
                                            const HexgridEllipsoid& ellipsoid) {
    const uint32_t level = hexgrid_detail::hexgrid_epoch_level(ellipsoid, HEXGRID_CELL_AREA_M2);
    const hexgrid_detail::HexgridCellRing ring = hexgrid_detail::hexgrid_cell_ring(cx, cz, level);
    return ring.valid ? ring.count : 0u;
}

/**
 * @brief Addresses of this cell's neighbours
 * @param cx Cell chunk X address
 * @param cz Cell chunk Z address
 * @param ellipsoid Reference ellipsoid parameters
 * @param[out] out_cx Neighbour X addresses, capacity HEXGRID_MAX_NEIGHBORS
 * @param[out] out_cz Neighbour Z addresses, capacity HEXGRID_MAX_NEIGHBORS
 * @param out_capacity Element capacity of both output buffers
 * @return Number of neighbour addresses written, 5 or 6
 *
 * MEMORY CONTRACT: the caller supplies both buffers with at least HEXGRID_MAX_NEIGHBORS elements.
 * Slot i of out_cx and slot i of out_cz form one address and share the edge index i, so the
 * neighbour order and the edge order are the same order. Nothing is written beyond the returned
 * count; unwritten slots keep whatever the caller put there. The order itself runs counter
 * clockwise around the cell, starting from the neighbour first met when sweeping from the
 * direction of the owning face centre, which makes it deterministic without being arbitrary.
 */
inline uint32_t hexgrid_cell_neighbors(int32_t cx, int32_t cz, const HexgridEllipsoid& ellipsoid,
                                       int32_t* out_cx, int32_t* out_cz, size_t out_capacity) {
    if (out_cx == nullptr || out_cz == nullptr) {
        return 0u;
    }
    const uint32_t level = hexgrid_detail::hexgrid_epoch_level(ellipsoid, HEXGRID_CELL_AREA_M2);
    const hexgrid_detail::HexgridCellRing ring = hexgrid_detail::hexgrid_cell_ring(cx, cz, level);
    if (!ring.valid || out_capacity < static_cast<size_t>(ring.count)) {
        return 0u;
    }
    for (uint32_t k = 0u; k < ring.count; ++k) {
        out_cx[k] = ring.neighbor_cx[k];
        out_cz[k] = ring.neighbor_cz[k];
    }
    return ring.count;
}

/**
 * @brief Geometry of one cell edge on the reference ellipsoid
 * @param cx Cell chunk X address
 * @param cz Cell chunk Z address
 * @param edge_index Edge ordinal, 0 to 5, sharing the ordering of hexgrid_cell_neighbors
 * @param ellipsoid Reference ellipsoid parameters
 * @param[out] start Cartesian start point of the edge, Y up
 * @param[out] end Cartesian end point of the edge, Y up
 * @return true when edge_index addresses an edge this cell actually has
 *
 * A pentagon cell has five edges, so edge_index 5 reports false there. Gates ride on these edges
 * but are cell entity data of the GIS phase, never geometry. The two endpoints are the cell
 * corners that bound the edge towards the neighbour of the same index; the edge itself is the
 * ellipsoid segment between them.
 */
inline bool hexgrid_cell_edge(int32_t cx, int32_t cz, uint32_t edge_index,
                              const HexgridEllipsoid& ellipsoid, Vec3& start, Vec3& end) {
    const uint32_t level = hexgrid_detail::hexgrid_epoch_level(ellipsoid, HEXGRID_CELL_AREA_M2);
    const hexgrid_detail::HexgridCellRing ring = hexgrid_detail::hexgrid_cell_ring(cx, cz, level);
    if (!ring.valid || edge_index >= ring.count) {
        return false;
    }
    const uint32_t before = (edge_index + ring.count - 1u) % ring.count;
    start = hexgrid_detail::hexgrid_surface_point(ring.corner[before], ellipsoid);
    end = hexgrid_detail::hexgrid_surface_point(ring.corner[edge_index], ellipsoid);
    return true;
}

// =============================================================================
// GROWTH - N(R) after Master DECISION D2 (option B plus C)
// =============================================================================

/**
 * @brief Surface area of the reference ellipsoid in square metres
 * @param ellipsoid Reference ellipsoid parameters
 * @return Surface area A(R) in square metres
 */
inline float hexgrid_ellipsoid_surface_area(const HexgridEllipsoid& ellipsoid) {
    return hexgrid_detail::hexgrid_surface_area(ellipsoid);
}

/**
 * @brief Cell count of the lattice at a given ellipsoid size
 * @param ellipsoid Reference ellipsoid parameters
 * @param cell_area_m2 Constant honeycomb area, normally HEXGRID_CELL_AREA_M2
 * @return Number of cells N(R)
 *
 * DECISION D2 option B: the cell count follows the ellipsoid surface at CONSTANT honeycomb area,
 * N(R) equals A(R) divided by A_cell. The subdivision frequency is never raised, so growth adds
 * cells as no man's land instead of shrinking every existing cell.
 *
 * The realised count is the count the lattice actually carries, ten times the squared frequency
 * plus the two that the icosahedron topology adds. At an epoch radius that is A(R) divided by
 * A_cell; between two epochs the radius has moved on while the lattice has not, which is exactly
 * what option C means by growing only at epochs.
 */
inline uint64_t hexgrid_cell_count(const HexgridEllipsoid& ellipsoid, float cell_area_m2) {
    const uint32_t level = hexgrid_detail::hexgrid_epoch_level(ellipsoid, cell_area_m2);
    const uint64_t frequency = static_cast<uint64_t>(1u) << level;
    return 10u * frequency * frequency + 2u;
}

/**
 * @brief Cell count added by one growth epoch
 * @param before Ellipsoid parameters at the start of the epoch
 * @param after Ellipsoid parameters at the end of the epoch
 * @param cell_area_m2 Constant honeycomb area, normally HEXGRID_CELL_AREA_M2
 * @return Number of cells the epoch adds, zero when the radius did not grow
 *
 * DECISION D2 option C: the radius grows in discrete epochs, so the increment is precomputable and
 * verifiable. Every added cell starts as no man's land; existing addresses and anchors are stable.
 */
inline uint64_t hexgrid_epoch_cell_increment(const HexgridEllipsoid& before,
                                             const HexgridEllipsoid& after, float cell_area_m2) {
    const uint64_t old_count = hexgrid_cell_count(before, cell_area_m2);
    const uint64_t new_count = hexgrid_cell_count(after, cell_area_m2);
    return (new_count > old_count) ? (new_count - old_count) : 0u;
}

// =============================================================================
// CONSTRUCTION CHAIN (DSGN_017, Z. 101-110) - the surface PHASE_01 builds against
// =============================================================================

/**
 * @brief Icosahedron base of the lattice
 * @param[out] out_vertices Unit vertex coordinates, flat [x0,y0,z0,x1,y1,z1,...]
 * @param vertex_capacity Element capacity of out_vertices, at least 3 times
 *                        HEXGRID_ICOSAHEDRON_VERTICES
 * @param[out] out_faces Triangle vertex indices, flat [i0,j0,k0,i1,j1,k1,...]
 * @param face_capacity Element capacity of out_faces, at least 3 times HEXGRID_ICOSAHEDRON_FACES
 * @return Number of vertices written, HEXGRID_ICOSAHEDRON_VERTICES on success and 0 when a
 *         capacity is short
 *
 * MEMORY CONTRACT: both buffers are caller owned. The vertex buffer needs 36 floats, the face
 * buffer 60 indices - both cardinalities are fixed by the solid and stated as constants above.
 * The twelve vertices ARE the pentagon seeds (Master binding decision 4). The faces are wound
 * counter clockwise as seen from outside, so the cross product of two edges points outward.
 */
inline uint32_t hexgrid_icosahedron_base(float* out_vertices, size_t vertex_capacity,
                                         uint32_t* out_faces, size_t face_capacity) {
    if (out_vertices == nullptr || out_faces == nullptr) {
        return 0u;
    }
    if (vertex_capacity < static_cast<size_t>(HEXGRID_ICOSAHEDRON_VERTICES) * 3u) {
        return 0u;
    }
    if (face_capacity < static_cast<size_t>(HEXGRID_ICOSAHEDRON_FACES) * 3u) {
        return 0u;
    }
    const hexgrid_detail::HexgridSolid& solid = hexgrid_detail::hexgrid_solid();
    for (uint32_t v = 0u; v < HEXGRID_ICOSAHEDRON_VERTICES; ++v) {
        out_vertices[v * 3u + 0u] = solid.vertex[v].x;
        out_vertices[v * 3u + 1u] = solid.vertex[v].y;
        out_vertices[v * 3u + 2u] = solid.vertex[v].z;
    }
    for (uint32_t f = 0u; f < HEXGRID_ICOSAHEDRON_FACES; ++f) {
        out_faces[f * 3u + 0u] = solid.face_vertex[f][0];
        out_faces[f * 3u + 1u] = solid.face_vertex[f][1];
        out_faces[f * 3u + 2u] = solid.face_vertex[f][2];
    }
    return HEXGRID_ICOSAHEDRON_VERTICES;
}

/**
 * @brief Parabolic lift of planar sites, z equals x squared plus y squared
 * @param sites_xy Input sites, flat [x0,y0,x1,y1,...]
 * @param site_count Number of input sites, NOT the array length
 * @param[out] out_xyz Lifted sites, flat [x0,y0,z0,x1,y1,z1,...]
 * @param out_capacity Element capacity of out_xyz, at least 3 times site_count
 * @return Number of lifted sites written, 0 when the capacity is short
 *
 * MEMORY CONTRACT: caller owned output of 3 times site_count floats; the input is read only.
 * The lift is what turns a 2D Delaunay problem into a 3D hull problem (DSGN_017, Z. 107, 110).
 */
inline size_t hexgrid_parabolic_projection(const float* sites_xy, size_t site_count,
                                           float* out_xyz, size_t out_capacity) {
    if (sites_xy == nullptr || out_xyz == nullptr) {
        return 0u;
    }
    if (out_capacity < site_count * 3u) {
        return 0u;
    }
    for (size_t s = 0u; s < site_count; ++s) {
        const float x = sites_xy[s * 2u + 0u];
        const float y = sites_xy[s * 2u + 1u];
        out_xyz[s * 3u + 0u] = x;
        out_xyz[s * 3u + 1u] = y;
        out_xyz[s * 3u + 2u] = x * x + y * y;
    }
    return site_count;
}

namespace hexgrid_detail {

/** @brief Read one site of a flat xyz span */
inline Vec3 hexgrid_site_at(const float* sites_xyz, size_t index) {
    return Vec3{sites_xyz[index * 3u + 0u], sites_xyz[index * 3u + 1u], sites_xyz[index * 3u + 2u]};
}

/**
 * @brief Pivot one open hull edge onto the site that closes it
 * @return Index of the apex site, or site_count when the edge cannot be closed
 *
 * The half plane that carries the current face is rotated about the edge, away from that face,
 * until it meets a site. The first site it meets is the apex of the adjacent hull face - this is
 * the gift wrapping step, and it needs no scratch memory, which is why it is the algorithm that
 * fits a Layer 0 memory contract of exactly one output buffer.
 */
inline size_t hexgrid_hull_pivot(const float* sites_xyz, size_t site_count, size_t edge_a,
                                 size_t edge_b, const Vec3& inward, const Vec3& outward) {
    const Vec3 point_a = hexgrid_site_at(sites_xyz, edge_a);
    const Vec3 point_b = hexgrid_site_at(sites_xyz, edge_b);
    const Vec3 along = normalize(point_b - point_a);
    size_t apex = site_count;
    float smallest = TWO_PI * 2.0f;
    for (size_t q = 0u; q < site_count; ++q) {
        if (q == edge_a || q == edge_b) {
            continue;
        }
        const Vec3 offset = hexgrid_site_at(sites_xyz, q) - point_a;
        const Vec3 perpendicular = offset - along * dot(offset, along);
        if (perpendicular.length_squared() < HEXGRID_HULL_EPSILON) {
            continue;
        }
        const Vec3 unit = normalize(perpendicular);
        float angle = atan2(-dot(unit, outward), dot(unit, inward));
        if (angle <= 0.0f) {
            angle += TWO_PI;
        }
        if (angle < smallest) {
            smallest = angle;
            apex = q;
        }
    }
    return apex;
}

/**
 * @brief First hull face of the gift wrapping run
 * @return false when the sites are degenerate, that is coplanar or lower dimensional
 *
 * The lexicographically smallest site is a hull vertex. A supporting plane through it is rotated
 * about the vertical until it catches a second hull vertex, then the resulting edge is pivoted
 * once more to catch the third. The result is verified against every site, so a face is only
 * accepted when it really supports the whole set.
 */
inline bool hexgrid_hull_seed(const float* sites_xyz, size_t site_count, uint32_t& out_a,
                              uint32_t& out_b, uint32_t& out_c) {
    size_t first = 0u;
    for (size_t s = 1u; s < site_count; ++s) {
        const Vec3 candidate = hexgrid_site_at(sites_xyz, s);
        const Vec3 current = hexgrid_site_at(sites_xyz, first);
        const bool smaller =
            (candidate.x < current.x) ||
            (candidate.x == current.x && candidate.y < current.y) ||
            (candidate.x == current.x && candidate.y == current.y && candidate.z < current.z);
        if (smaller) {
            first = s;
        }
    }

    const Vec3 origin = hexgrid_site_at(sites_xyz, first);
    size_t second = site_count;
    float smallest = TWO_PI;
    float farthest = -1.0f;
    for (size_t s = 0u; s < site_count; ++s) {
        if (s == first) {
            continue;
        }
        const Vec3 offset = hexgrid_site_at(sites_xyz, s) - origin;
        const float planar = offset.x * offset.x + offset.y * offset.y;
        if (planar < HEXGRID_HULL_EPSILON) {
            continue;
        }
        const float angle = atan2(offset.y, offset.x) + HALF_PI;
        if (angle < smallest - HEXGRID_HULL_EPSILON ||
            (abs(angle - smallest) <= HEXGRID_HULL_EPSILON && planar > farthest)) {
            smallest = min(smallest, angle);
            farthest = planar;
            second = s;
        }
    }
    if (second >= site_count) {
        return false;
    }

    const Vec3 support = Vec3{-cos(smallest), -sin(smallest), 0.0f};
    const Vec3 along = normalize(hexgrid_site_at(sites_xyz, second) - origin);
    const Vec3 inward = cross(support, along);
    const size_t third = hexgrid_hull_pivot(sites_xyz, site_count, first, second, inward, support);
    if (third >= site_count) {
        return false;
    }

    const Vec3 point_c = hexgrid_site_at(sites_xyz, third);
    Vec3 normal = cross(hexgrid_site_at(sites_xyz, second) - origin, point_c - origin);
    if (normal.length_squared() < HEXGRID_HULL_EPSILON) {
        return false;
    }
    normal = normalize(normal);
    bool above = false;
    bool below = false;
    for (size_t s = 0u; s < site_count; ++s) {
        const float side = dot(normal, hexgrid_site_at(sites_xyz, s) - origin);
        if (side > 1.0e-5f) {
            above = true;
        }
        if (side < -1.0e-5f) {
            below = true;
        }
    }
    if (above && below) {
        return false;
    }
    out_a = static_cast<uint32_t>(first);
    out_b = static_cast<uint32_t>(above ? third : second);
    out_c = static_cast<uint32_t>(above ? second : third);
    return true;
}

}  // namespace hexgrid_detail

/**
 * @brief Convex hull of lifted sites
 * @param sites_xyz Input sites, flat [x0,y0,z0,x1,y1,z1,...]
 * @param site_count Number of input sites, NOT the array length
 * @param[out] out_faces Hull triangle vertex indices, flat [i0,j0,k0,...]
 * @param face_capacity Element capacity of out_faces, at least 3 times
 *                      hexgrid_hull_face_capacity(site_count)
 * @return Number of hull triangles written, 0 when the capacity is short or the sites are
 *         degenerate
 *
 * MEMORY CONTRACT: the face buffer is caller owned and sized from
 * hexgrid_hull_face_capacity(site_count) times 3. Indices refer to the INPUT site order, so the
 * caller keeps its site array alive and needs no vertex copy.
 *
 * Gift wrapping is what a single output buffer allows: every face is written exactly once and is
 * never revisited, so the buffer never has to hold more than the final hull. An open edge is one
 * whose reverse has not been emitted yet, which is read straight out of the buffer instead of out
 * of an adjacency structure the memory contract has no room for. The cost of that is quadratic in
 * the face count plus linear per pivot, which is the honest price of allocating nothing.
 */
inline size_t hexgrid_convex_hull(const float* sites_xyz, size_t site_count,
                                  uint32_t* out_faces, size_t face_capacity) {
    if (sites_xyz == nullptr || out_faces == nullptr || site_count < 4u) {
        return 0u;
    }
    if (face_capacity < hexgrid_hull_face_capacity(site_count) * 3u) {
        return 0u;
    }
    uint32_t seed_a = 0u;
    uint32_t seed_b = 0u;
    uint32_t seed_c = 0u;
    if (!hexgrid_detail::hexgrid_hull_seed(sites_xyz, site_count, seed_a, seed_b, seed_c)) {
        return 0u;
    }
    out_faces[0] = seed_a;
    out_faces[1] = seed_b;
    out_faces[2] = seed_c;
    size_t face_count = 1u;

    for (size_t cursor = 0u; cursor < face_count; ++cursor) {
        const uint32_t corner[3] = {out_faces[cursor * 3u + 0u], out_faces[cursor * 3u + 1u],
                                    out_faces[cursor * 3u + 2u]};
        for (uint32_t e = 0u; e < 3u; ++e) {
            const uint32_t edge_a = corner[e];
            const uint32_t edge_b = corner[(e + 1u) % 3u];
            const uint32_t edge_c = corner[(e + 2u) % 3u];
            bool closed = false;
            for (size_t t = 0u; t < face_count && !closed; ++t) {
                const uint32_t other[3] = {out_faces[t * 3u + 0u], out_faces[t * 3u + 1u],
                                           out_faces[t * 3u + 2u]};
                for (uint32_t f = 0u; f < 3u; ++f) {
                    if (other[f] == edge_b && other[(f + 1u) % 3u] == edge_a) {
                        closed = true;
                        break;
                    }
                }
            }
            if (closed) {
                continue;
            }
            const Vec3 point_a = hexgrid_detail::hexgrid_site_at(sites_xyz, edge_a);
            const Vec3 point_b = hexgrid_detail::hexgrid_site_at(sites_xyz, edge_b);
            const Vec3 point_c = hexgrid_detail::hexgrid_site_at(sites_xyz, edge_c);
            const Vec3 along = normalize(point_b - point_a);
            const Vec3 offset = point_c - point_a;
            const Vec3 inward = normalize(offset - along * dot(offset, along));
            const Vec3 outward = cross(along, inward);
            const size_t apex =
                hexgrid_detail::hexgrid_hull_pivot(sites_xyz, site_count, edge_a, edge_b, inward,
                                                   outward);
            if (apex >= site_count) {
                continue;
            }
            if ((face_count + 1u) * 3u > face_capacity) {
                return 0u;
            }
            out_faces[face_count * 3u + 0u] = edge_b;
            out_faces[face_count * 3u + 1u] = edge_a;
            out_faces[face_count * 3u + 2u] = static_cast<uint32_t>(apex);
            ++face_count;
        }
    }
    return face_count;
}

/**
 * @brief Delaunay triangulation from the downward oriented hull faces
 * @param sites_xyz Lifted input sites, flat [x0,y0,z0,...]
 * @param site_count Number of input sites
 * @param hull_faces Hull triangle indices as produced by hexgrid_convex_hull
 * @param hull_face_count Number of hull triangles
 * @param[out] out_triangles Delaunay triangle vertex indices, flat [i0,j0,k0,...]
 * @param triangle_capacity Element capacity of out_triangles, at least 3 times
 *                          hexgrid_delaunay_triangle_capacity(site_count)
 * @return Number of Delaunay triangles written, 0 when the capacity is short
 *
 * MEMORY CONTRACT: caller owned output sized from hexgrid_delaunay_triangle_capacity. Only the
 * DOWNWARD oriented hull triangles carry the Delaunay triangulation (DSGN_017, Z. 110); the upward
 * ones are dropped, which is why the written count is normally below hull_face_count.
 */
inline size_t hexgrid_delaunay_triangulation(const float* sites_xyz, size_t site_count,
                                             const uint32_t* hull_faces, size_t hull_face_count,
                                             uint32_t* out_triangles, size_t triangle_capacity) {
    if (sites_xyz == nullptr || hull_faces == nullptr || out_triangles == nullptr) {
        return 0u;
    }
    if (triangle_capacity < hexgrid_delaunay_triangle_capacity(site_count) * 3u) {
        return 0u;
    }
    size_t written = 0u;
    for (size_t f = 0u; f < hull_face_count; ++f) {
        const uint32_t index_a = hull_faces[f * 3u + 0u];
        const uint32_t index_b = hull_faces[f * 3u + 1u];
        const uint32_t index_c = hull_faces[f * 3u + 2u];
        const Vec3 point_a = hexgrid_detail::hexgrid_site_at(sites_xyz, index_a);
        const Vec3 point_b = hexgrid_detail::hexgrid_site_at(sites_xyz, index_b);
        const Vec3 point_c = hexgrid_detail::hexgrid_site_at(sites_xyz, index_c);
        const Vec3 normal = cross(point_b - point_a, point_c - point_a);
        if (normal.z >= 0.0f) {
            continue;
        }
        if ((written + 1u) * 3u > triangle_capacity) {
            return 0u;
        }
        out_triangles[written * 3u + 0u] = index_a;
        out_triangles[written * 3u + 1u] = index_b;
        out_triangles[written * 3u + 2u] = index_c;
        ++written;
    }
    return written;
}

/**
 * @brief Voronoi dual of a Delaunay triangulation - the honeycomb itself
 * @param sites_xyz Input sites, flat [x0,y0,z0,...]
 * @param site_count Number of input sites
 * @param triangles Delaunay triangle indices as produced by hexgrid_delaunay_triangulation
 * @param triangle_count Number of Delaunay triangles
 * @param[out] out_vertices Voronoi vertex coordinates, flat [x0,y0,z0,...], capacity from
 *                          hexgrid_voronoi_vertex_capacity(site_count) times 3
 * @param vertex_capacity Element capacity of out_vertices
 * @param[out] out_ring_indices Flat per cell ring of Voronoi vertex indices, capacity from
 *                              hexgrid_voronoi_index_capacity(site_count)
 * @param ring_capacity Element capacity of out_ring_indices
 * @param[out] out_ring_counts Ring length per cell, 5 or 6, capacity site_count
 * @param ring_count_capacity Element capacity of out_ring_counts
 * @return Number of Voronoi vertices written, 0 when a capacity is short
 *
 * MEMORY CONTRACT: three caller owned buffers, each sized from the constexpr helpers above. The
 * ring buffer is FLAT with a fixed stride of HEXGRID_MAX_NEIGHBORS: cell i owns the slots
 * [i * HEXGRID_MAX_NEIGHBORS, i * HEXGRID_MAX_NEIGHBORS + out_ring_counts[i]). A pentagon cell
 * leaves its sixth slot untouched, which is exactly why the per cell count is reported separately
 * instead of being inferred from the stride.
 *
 * A Voronoi vertex is the circumcentre of a Delaunay triangle in the plane the sites were lifted
 * from, so the third coordinate is zero. Each ring is sorted counter clockwise around its site.
 * A site whose Voronoi degree exceeds the frozen stride cannot be laid out in this buffer shape,
 * and the function reports zero rather than truncating a ring into a wrong polygon.
 */
inline size_t hexgrid_voronoi_dual(const float* sites_xyz, size_t site_count,
                                   const uint32_t* triangles, size_t triangle_count,
                                   float* out_vertices, size_t vertex_capacity,
                                   uint32_t* out_ring_indices, size_t ring_capacity,
                                   uint32_t* out_ring_counts, size_t ring_count_capacity) {
    if (sites_xyz == nullptr || triangles == nullptr || out_vertices == nullptr) {
        return 0u;
    }
    if (out_ring_indices == nullptr || out_ring_counts == nullptr) {
        return 0u;
    }
    if (vertex_capacity < triangle_count * 3u) {
        return 0u;
    }
    if (ring_capacity < hexgrid_voronoi_index_capacity(site_count) ||
        ring_count_capacity < site_count) {
        return 0u;
    }

    for (size_t t = 0u; t < triangle_count; ++t) {
        const Vec3 point_a = hexgrid_detail::hexgrid_site_at(sites_xyz, triangles[t * 3u + 0u]);
        const Vec3 point_b = hexgrid_detail::hexgrid_site_at(sites_xyz, triangles[t * 3u + 1u]);
        const Vec3 point_c = hexgrid_detail::hexgrid_site_at(sites_xyz, triangles[t * 3u + 2u]);
        const float twice_area = 2.0f * (point_a.x * (point_b.y - point_c.y) +
                                         point_b.x * (point_c.y - point_a.y) +
                                         point_c.x * (point_a.y - point_b.y));
        float center_x = point_a.x;
        float center_y = point_a.y;
        if (abs(twice_area) > HEXGRID_HULL_EPSILON) {
            const float len_a = point_a.x * point_a.x + point_a.y * point_a.y;
            const float len_b = point_b.x * point_b.x + point_b.y * point_b.y;
            const float len_c = point_c.x * point_c.x + point_c.y * point_c.y;
            center_x = (len_a * (point_b.y - point_c.y) + len_b * (point_c.y - point_a.y) +
                        len_c * (point_a.y - point_b.y)) / twice_area;
            center_y = (len_a * (point_c.x - point_b.x) + len_b * (point_a.x - point_c.x) +
                        len_c * (point_b.x - point_a.x)) / twice_area;
        }
        out_vertices[t * 3u + 0u] = center_x;
        out_vertices[t * 3u + 1u] = center_y;
        out_vertices[t * 3u + 2u] = 0.0f;
    }

    for (size_t s = 0u; s < site_count; ++s) {
        out_ring_counts[s] = 0u;
    }
    for (size_t t = 0u; t < triangle_count; ++t) {
        for (uint32_t c = 0u; c < 3u; ++c) {
            const uint32_t site = triangles[t * 3u + c];
            if (static_cast<size_t>(site) >= site_count) {
                return 0u;
            }
            const uint32_t used = out_ring_counts[site];
            if (used >= HEXGRID_MAX_NEIGHBORS) {
                return 0u;
            }
            out_ring_indices[static_cast<size_t>(site) * HEXGRID_MAX_NEIGHBORS + used] =
                static_cast<uint32_t>(t);
            out_ring_counts[site] = used + 1u;
        }
    }

    for (size_t s = 0u; s < site_count; ++s) {
        const uint32_t used = out_ring_counts[s];
        if (used < 2u) {
            continue;
        }
        const Vec3 site_point = hexgrid_detail::hexgrid_site_at(sites_xyz, s);
        float key[HEXGRID_MAX_NEIGHBORS];
        uint32_t order[HEXGRID_MAX_NEIGHBORS];
        for (uint32_t k = 0u; k < used; ++k) {
            const uint32_t vertex = out_ring_indices[s * HEXGRID_MAX_NEIGHBORS + k];
            key[k] = atan2(out_vertices[vertex * 3u + 1u] - site_point.y,
                           out_vertices[vertex * 3u + 0u] - site_point.x);
            order[k] = vertex;
        }
        hexgrid_detail::hexgrid_sort_by_key(key, order, used);
        for (uint32_t k = 0u; k < used; ++k) {
            out_ring_indices[s * HEXGRID_MAX_NEIGHBORS + k] = order[k];
        }
    }
    return triangle_count;
}

/**
 * @brief Centroid relaxation towards the idealised lattice spacing
 * @param[in,out] sites_xyz Sites to relax in place, flat [x0,y0,z0,...]
 * @param site_count Number of sites
 * @param ring_indices Flat Voronoi rings as produced by hexgrid_voronoi_dual
 * @param ring_counts Ring length per cell
 * @param vertices Voronoi vertex coordinates as produced by hexgrid_voronoi_dual
 * @param vertex_count Number of Voronoi vertices
 * @param first_mutable_site Index of the first site the pass may move
 * @param max_iterations Iteration ceiling, normally HEXGRID_RELAX_MAX_ITERATIONS
 * @return Number of iterations actually run
 *
 * MEMORY CONTRACT: the site buffer is caller owned and is MUTATED in place; no other buffer is
 * written. Nothing is allocated.
 *
 * LOCALITY: sites below first_mutable_site are held fixed. This is how growth stays address stable
 * - a growth pass passes the index of the first newly added site, and the relaxation then moves
 * only the new cells and the cells within HEXGRID_RELAX_RING_DEPTH rings around them, leaving
 * established anchors alone (Master DECISION D2 consequence clause).
 * Mechanism source: DSGN_017, Z. 109-110 (area centroids for the idealised lattice spacing); the
 * term relaxation is this plan's name for that mechanism, it is not quoted from DSGN_017.
 */
inline uint32_t hexgrid_centroid_relaxation(float* sites_xyz, size_t site_count,
                                            const uint32_t* ring_indices,
                                            const uint32_t* ring_counts, const float* vertices,
                                            size_t vertex_count, size_t first_mutable_site,
                                            uint32_t max_iterations) {
    if (sites_xyz == nullptr || ring_indices == nullptr || ring_counts == nullptr) {
        return 0u;
    }
    if (vertices == nullptr || vertex_count == 0u) {
        return 0u;
    }
    uint32_t iterations = 0u;
    while (iterations < max_iterations) {
        for (size_t s = first_mutable_site; s < site_count; ++s) {
            const uint32_t used = ring_counts[s];
            if (used < 3u) {
                continue;
            }
            float twice_area = 0.0f;
            float centroid_x = 0.0f;
            float centroid_y = 0.0f;
            for (uint32_t k = 0u; k < used; ++k) {
                const uint32_t current = ring_indices[s * HEXGRID_MAX_NEIGHBORS + k];
                const uint32_t following = ring_indices[s * HEXGRID_MAX_NEIGHBORS + ((k + 1u) % used)];
                if (static_cast<size_t>(current) >= vertex_count ||
                    static_cast<size_t>(following) >= vertex_count) {
                    continue;
                }
                const float x0 = vertices[current * 3u + 0u];
                const float y0 = vertices[current * 3u + 1u];
                const float x1 = vertices[following * 3u + 0u];
                const float y1 = vertices[following * 3u + 1u];
                const float cross_term = x0 * y1 - x1 * y0;
                twice_area += cross_term;
                centroid_x += (x0 + x1) * cross_term;
                centroid_y += (y0 + y1) * cross_term;
            }
            if (abs(twice_area) < HEXGRID_HULL_EPSILON) {
                continue;
            }
            sites_xyz[s * 3u + 0u] = centroid_x / (3.0f * twice_area);
            sites_xyz[s * 3u + 1u] = centroid_y / (3.0f * twice_area);
            sites_xyz[s * 3u + 2u] = sites_xyz[s * 3u + 0u] * sites_xyz[s * 3u + 0u] +
                                     sites_xyz[s * 3u + 1u] * sites_xyz[s * 3u + 1u];
        }
        ++iterations;
    }
    return iterations;
}

/**
 * @brief Area of one Voronoi cell on the reference ellipsoid
 * @param ring_indices Flat Voronoi rings as produced by hexgrid_voronoi_dual
 * @param ring_counts Ring length per cell
 * @param vertices Voronoi vertex coordinates
 * @param site_index Index of the cell to measure
 * @return Cell area in square metres
 *
 * The equal distribution check compares this against HEXGRID_CELL_AREA_M2 within
 * HEXGRID_AREA_TOLERANCE.
 *
 * The sites live in the equal area face plane, whose metric IS the metric of the ellipsoid
 * surface, so the planar polygon area of a ring is the area of the cell on the ellipsoid and needs
 * no correction term. A ring shorter than three vertices belongs to an unbounded boundary cell and
 * reports zero, because an unbounded cell has no area to report.
 */
inline float hexgrid_cell_area(const uint32_t* ring_indices, const uint32_t* ring_counts,
                               const float* vertices, size_t site_index) {
    if (ring_indices == nullptr || ring_counts == nullptr || vertices == nullptr) {
        return 0.0f;
    }
    const uint32_t used = ring_counts[site_index];
    if (used < 3u) {
        return 0.0f;
    }
    float twice_area = 0.0f;
    for (uint32_t k = 0u; k < used; ++k) {
        const uint32_t current = ring_indices[site_index * HEXGRID_MAX_NEIGHBORS + k];
        const uint32_t following = ring_indices[site_index * HEXGRID_MAX_NEIGHBORS + ((k + 1u) % used)];
        const float x0 = vertices[current * 3u + 0u];
        const float y0 = vertices[current * 3u + 1u];
        const float x1 = vertices[following * 3u + 0u];
        const float y1 = vertices[following * 3u + 1u];
        twice_area += x0 * y1 - x1 * y0;
    }
    return abs(twice_area) * 0.5f;
}

}  // namespace ase::math
