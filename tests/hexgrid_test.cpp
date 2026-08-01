/**
 * Planet Hex Lattice Geometry Unit Tests
 *
 * The seven cases of PLAN_ASE_LATTICE_PHASE_01_GEOM.md: determinism, the twelve pentagons,
 * neighbour symmetry, the area tolerance band, the pole rule, the round trip, and growth.
 *
 * Every case walks the lattice through the frozen public surface of hexgrid.hpp only. A level is
 * addressed by handing in the sphere whose surface carries exactly that lattice at the nominal
 * honeycomb area, which is how the epoch reaches the queries: the ellipsoid IS the epoch parameter.
 *
 * The doctest main is switched on from tests/CMakeLists.txt rather than from a define here, so
 * this file carries no macro definition of its own.
 */

#include <doctest/doctest.h>

#include <ase/math/hexgrid.hpp>

using namespace ase::math;

namespace {

constexpr uint32_t TEST_LEVEL = 4u;        // 2562 cells, the exhaustive sweep of most cases
constexpr uint32_t GROWTH_LEVEL = 3u;      // 642 cells, the epoch before TEST_LEVEL
constexpr size_t CELL_CAPACITY = 2600u;    // above 10 * 16 * 16 + 2
constexpr size_t SMALL_CAPACITY = 700u;    // above 10 * 8 * 8 + 2

/** The collected lattice of one level: canonical addresses plus their geodetic anchors. */
struct CellTable {
    int32_t cx[CELL_CAPACITY];
    int32_t cz[CELL_CAPACITY];
    float latitude[CELL_CAPACITY];
    float longitude[CELL_CAPACITY];
    size_t count;
};

/** The smaller table of the epoch below, used by the growth case. */
struct SmallCellTable {
    int32_t cx[SMALL_CAPACITY];
    int32_t cz[SMALL_CAPACITY];
    size_t count;
};

/** The sphere whose surface carries exactly the lattice of one level at the nominal cell area. */
HexgridEllipsoid sphere_for_level(uint32_t level) {
    const float frequency = static_cast<float>(1u << level);
    const float area = 10.0f * frequency * frequency * HEXGRID_CELL_AREA_M2;
    HexgridEllipsoid ellipsoid{};
    ellipsoid.semi_major_axis_m = sqrt(area / (4.0f * PI)) * 1.0001f;
    ellipsoid.flattening = 0.0f;
    return ellipsoid;
}

/** Expected cell count of a level: ten times the squared frequency plus the icosahedron pair. */
uint64_t expected_cell_count(uint32_t level) {
    const uint64_t frequency = static_cast<uint64_t>(1u) << level;
    return 10u * frequency * frequency + 2u;
}

/**
 * Walk every face local site of a level and collect the distinct canonical addresses.
 *
 * The canonical address is taken through the public surface, so a site that several faces share
 * collapses to one entry exactly as a consumer would see it.
 */
void fill_cell_table(uint32_t level, CellTable& table) {
    const HexgridEllipsoid ellipsoid = sphere_for_level(level);
    const int32_t step = HEXGRID_ADDRESS_FREQUENCY >> level;
    const int32_t frequency = static_cast<int32_t>(1u << level);
    table.count = 0u;
    for (uint32_t face = 0u; face < HEXGRID_ICOSAHEDRON_FACES; ++face) {
        for (int32_t i = 0; i <= frequency; ++i) {
            for (int32_t j = 0; j <= frequency - i; ++j) {
                const int32_t cx = static_cast<int32_t>(face) * HEXGRID_FACE_STRIDE + i * step;
                const int32_t cz = j * step;
                float latitude = 0.0f;
                float longitude = 0.0f;
                hexgrid_cell_to_geodetic(cx, cz, ellipsoid, latitude, longitude);
                int32_t canonical_cx = 0;
                int32_t canonical_cz = 0;
                hexgrid_geodetic_to_cell(latitude, longitude, ellipsoid, canonical_cx,
                                         canonical_cz);
                bool seen = false;
                for (size_t t = 0u; t < table.count; ++t) {
                    if (table.cx[t] == canonical_cx && table.cz[t] == canonical_cz) {
                        seen = true;
                        break;
                    }
                }
                if (seen || table.count >= CELL_CAPACITY) {
                    continue;
                }
                table.cx[table.count] = canonical_cx;
                table.cz[table.count] = canonical_cz;
                hexgrid_cell_to_geodetic(canonical_cx, canonical_cz, ellipsoid,
                                         table.latitude[table.count],
                                         table.longitude[table.count]);
                ++table.count;
            }
        }
    }
}

/** Same walk, into the smaller table of the epoch below. */
void fill_small_table(uint32_t level, SmallCellTable& table) {
    const HexgridEllipsoid ellipsoid = sphere_for_level(level);
    const int32_t step = HEXGRID_ADDRESS_FREQUENCY >> level;
    const int32_t frequency = static_cast<int32_t>(1u << level);
    table.count = 0u;
    for (uint32_t face = 0u; face < HEXGRID_ICOSAHEDRON_FACES; ++face) {
        for (int32_t i = 0; i <= frequency; ++i) {
            for (int32_t j = 0; j <= frequency - i; ++j) {
                const int32_t cx = static_cast<int32_t>(face) * HEXGRID_FACE_STRIDE + i * step;
                const int32_t cz = j * step;
                float latitude = 0.0f;
                float longitude = 0.0f;
                hexgrid_cell_to_geodetic(cx, cz, ellipsoid, latitude, longitude);
                int32_t canonical_cx = 0;
                int32_t canonical_cz = 0;
                hexgrid_geodetic_to_cell(latitude, longitude, ellipsoid, canonical_cx,
                                         canonical_cz);
                bool seen = false;
                for (size_t t = 0u; t < table.count; ++t) {
                    if (table.cx[t] == canonical_cx && table.cz[t] == canonical_cz) {
                        seen = true;
                        break;
                    }
                }
                if (seen || table.count >= SMALL_CAPACITY) {
                    continue;
                }
                table.cx[table.count] = canonical_cx;
                table.cz[table.count] = canonical_cz;
                ++table.count;
            }
        }
    }
}

/** Area of a spherical triangle over unit directions, stable for small and large triangles. */
float spherical_triangle_area(const Vec3& a, const Vec3& b, const Vec3& c) {
    const float numerator = abs(dot(a, cross(b, c)));
    const float denominator = 1.0f + dot(a, b) + dot(b, c) + dot(c, a);
    return 2.0f * atan2(numerator, denominator);
}

/** Area of one lattice cell in square metres, fanned from its centre over its own edges. */
float measured_cell_area(int32_t cx, int32_t cz, const HexgridEllipsoid& ellipsoid) {
    const uint32_t neighbors = hexgrid_cell_neighbor_count(cx, cz, ellipsoid);
    if (neighbors < HEXGRID_PENTAGON_NEIGHBORS) {
        return 0.0f;
    }
    const Vec3 center = normalize(hexgrid_cell_to_cartesian(cx, cz, ellipsoid));
    Vec3 corner[HEXGRID_MAX_NEIGHBORS];
    for (uint32_t k = 0u; k < neighbors; ++k) {
        Vec3 start{};
        Vec3 end{};
        if (!hexgrid_cell_edge(cx, cz, k, ellipsoid, start, end)) {
            return 0.0f;
        }
        corner[k] = normalize(end);
    }
    float solid_angle = 0.0f;
    for (uint32_t k = 0u; k < neighbors; ++k) {
        solid_angle += spherical_triangle_area(center, corner[k], corner[(k + 1u) % neighbors]);
    }
    const float radius = ellipsoid.semi_major_axis_m;
    return solid_angle * radius * radius;
}

}  // namespace

TEST_CASE("Hexgrid - determinism: the same lattice is built bit for bit") {
    CellTable first{};
    CellTable second{};
    fill_cell_table(TEST_LEVEL, first);
    fill_cell_table(TEST_LEVEL, second);
    REQUIRE(first.count > 0u);
    CHECK(second.count == first.count);

    bool identical = true;
    for (size_t t = 0u; t < first.count; ++t) {
        if (first.cx[t] != second.cx[t] || first.cz[t] != second.cz[t]) {
            identical = false;
        }
        if (first.latitude[t] != second.latitude[t] ||
            first.longitude[t] != second.longitude[t]) {
            identical = false;
        }
    }
    CHECK(identical);

    SUBCASE("the orientation seed is a constant of the header, so the solid repeats exactly") {
        float vertices_a[HEXGRID_ICOSAHEDRON_VERTICES * 3u] = {};
        float vertices_b[HEXGRID_ICOSAHEDRON_VERTICES * 3u] = {};
        uint32_t faces_a[HEXGRID_ICOSAHEDRON_FACES * 3u] = {};
        uint32_t faces_b[HEXGRID_ICOSAHEDRON_FACES * 3u] = {};
        CHECK(hexgrid_icosahedron_base(vertices_a, HEXGRID_ICOSAHEDRON_VERTICES * 3u, faces_a,
                                       HEXGRID_ICOSAHEDRON_FACES * 3u) ==
              HEXGRID_ICOSAHEDRON_VERTICES);
        CHECK(hexgrid_icosahedron_base(vertices_b, HEXGRID_ICOSAHEDRON_VERTICES * 3u, faces_b,
                                       HEXGRID_ICOSAHEDRON_FACES * 3u) ==
              HEXGRID_ICOSAHEDRON_VERTICES);
        bool solid_identical = true;
        for (uint32_t t = 0u; t < HEXGRID_ICOSAHEDRON_VERTICES * 3u; ++t) {
            if (vertices_a[t] != vertices_b[t]) {
                solid_identical = false;
            }
        }
        for (uint32_t t = 0u; t < HEXGRID_ICOSAHEDRON_FACES * 3u; ++t) {
            if (faces_a[t] != faces_b[t]) {
                solid_identical = false;
            }
        }
        CHECK(solid_identical);
    }
}

TEST_CASE("Hexgrid - exactly twelve pentagons, every other cell carries six neighbours") {
    const HexgridEllipsoid ellipsoid = sphere_for_level(TEST_LEVEL);
    CellTable table{};
    fill_cell_table(TEST_LEVEL, table);
    REQUIRE(table.count == static_cast<size_t>(expected_cell_count(TEST_LEVEL)));

    uint32_t pentagons = 0u;
    uint32_t hexagons = 0u;
    uint32_t predicate_matches = 0u;
    bool counts_are_five_or_six = true;
    for (size_t t = 0u; t < table.count; ++t) {
        const uint32_t neighbors = hexgrid_cell_neighbor_count(table.cx[t], table.cz[t], ellipsoid);
        const bool is_pentagon = hexgrid_cell_is_pentagon(table.cx[t], table.cz[t], ellipsoid);
        if (neighbors == HEXGRID_PENTAGON_NEIGHBORS) {
            ++pentagons;
        } else if (neighbors == HEXGRID_MAX_NEIGHBORS) {
            ++hexagons;
        } else {
            counts_are_five_or_six = false;
        }
        if (is_pentagon == (neighbors == HEXGRID_PENTAGON_NEIGHBORS)) {
            ++predicate_matches;
        }
    }
    CHECK(counts_are_five_or_six);
    CHECK(pentagons == HEXGRID_PENTAGON_COUNT);
    CHECK(hexagons == table.count - HEXGRID_PENTAGON_COUNT);
    CHECK(predicate_matches == table.count);
}

TEST_CASE("Hexgrid - neighbourhood is symmetric across every cell") {
    const HexgridEllipsoid ellipsoid = sphere_for_level(TEST_LEVEL);
    CellTable table{};
    fill_cell_table(TEST_LEVEL, table);
    REQUIRE(table.count == static_cast<size_t>(expected_cell_count(TEST_LEVEL)));

    bool symmetric = true;
    bool neighbours_are_distinct = true;
    for (size_t t = 0u; t < table.count; ++t) {
        int32_t neighbor_cx[HEXGRID_MAX_NEIGHBORS] = {};
        int32_t neighbor_cz[HEXGRID_MAX_NEIGHBORS] = {};
        const uint32_t written = hexgrid_cell_neighbors(table.cx[t], table.cz[t], ellipsoid,
                                                        neighbor_cx, neighbor_cz,
                                                        HEXGRID_MAX_NEIGHBORS);
        REQUIRE(written >= HEXGRID_PENTAGON_NEIGHBORS);
        for (uint32_t a = 0u; a < written; ++a) {
            for (uint32_t b = a + 1u; b < written; ++b) {
                if (neighbor_cx[a] == neighbor_cx[b] && neighbor_cz[a] == neighbor_cz[b]) {
                    neighbours_are_distinct = false;
                }
            }
            int32_t back_cx[HEXGRID_MAX_NEIGHBORS] = {};
            int32_t back_cz[HEXGRID_MAX_NEIGHBORS] = {};
            const uint32_t back = hexgrid_cell_neighbors(neighbor_cx[a], neighbor_cz[a], ellipsoid,
                                                         back_cx, back_cz, HEXGRID_MAX_NEIGHBORS);
            bool found = false;
            for (uint32_t c = 0u; c < back; ++c) {
                if (back_cx[c] == table.cx[t] && back_cz[c] == table.cz[t]) {
                    found = true;
                }
            }
            if (!found) {
                symmetric = false;
            }
        }
    }
    CHECK(neighbours_are_distinct);
    CHECK(symmetric);
}

TEST_CASE("Hexgrid - every cell area stays inside the contract tolerance band") {
    const HexgridEllipsoid ellipsoid = sphere_for_level(TEST_LEVEL);
    CellTable table{};
    fill_cell_table(TEST_LEVEL, table);
    REQUIRE(table.count == static_cast<size_t>(expected_cell_count(TEST_LEVEL)));

    // The nominal hexagon area follows from the tiling itself: the twelve pentagons carry five
    // sixths of a hexagon each, so ten times the squared frequency hexagons fill the surface.
    const float frequency = static_cast<float>(1u << TEST_LEVEL);
    const float surface = hexgrid_ellipsoid_surface_area(ellipsoid);
    const float hexagon_area = surface / (10.0f * frequency * frequency);
    const float pentagon_area = hexagon_area * HEXGRID_PENTAGON_AREA_RATIO;

    uint32_t inside_band = 0u;
    float summed = 0.0f;
    for (size_t t = 0u; t < table.count; ++t) {
        const bool is_pentagon = hexgrid_cell_is_pentagon(table.cx[t], table.cz[t], ellipsoid);
        const float nominal = is_pentagon ? pentagon_area : hexagon_area;
        const float measured = measured_cell_area(table.cx[t], table.cz[t], ellipsoid);
        summed += measured;
        if (abs(measured - nominal) <= nominal * HEXGRID_AREA_TOLERANCE) {
            ++inside_band;
        }
    }
    CHECK(inside_band == table.count);

    SUBCASE("the cells tile the ellipsoid without gap or overlap") {
        CHECK(abs(summed / surface - 1.0f) < 0.01f);
    }

    SUBCASE("the mirrored chunk area is pinned against the honeycomb area") {
        CHECK(HEXGRID_CELL_EDGE_M == 32.0f);
        CHECK(HEXGRID_CELL_AREA_M2 == 1024.0f);
        CHECK(abs(hexagon_area / HEXGRID_CELL_AREA_M2 - 1.0f) < HEXGRID_AREA_TOLERANCE);
    }
}

TEST_CASE("Hexgrid - no pentagon sits on a pole and the polar caps are ordinary cells") {
    const HexgridEllipsoid ellipsoid = sphere_for_level(TEST_LEVEL);
    CellTable table{};
    fill_cell_table(TEST_LEVEL, table);
    REQUIRE(table.count == static_cast<size_t>(expected_cell_count(TEST_LEVEL)));

    float closest_pentagon_to_pole = 90.0f;
    for (size_t t = 0u; t < table.count; ++t) {
        if (!hexgrid_cell_is_pentagon(table.cx[t], table.cz[t], ellipsoid)) {
            continue;
        }
        const float distance = 90.0f - abs(table.latitude[t]);
        if (distance < closest_pentagon_to_pole) {
            closest_pentagon_to_pole = distance;
        }
    }
    // The orientation seed puts a face centre on the pole, which is the largest separation the
    // solid allows: the nearest icosahedron vertex is then a face circumradius away.
    CHECK(closest_pentagon_to_pole > 30.0f);

    SUBCASE("both poles fall into an ordinary six sided cell") {
        for (float pole_latitude = -90.0f; pole_latitude < 91.0f; pole_latitude += 180.0f) {
            int32_t pole_cx = 0;
            int32_t pole_cz = 0;
            hexgrid_geodetic_to_cell(pole_latitude, 0.0f, ellipsoid, pole_cx, pole_cz);
            CHECK_FALSE(hexgrid_cell_is_pentagon(pole_cx, pole_cz, ellipsoid));
            CHECK(hexgrid_cell_neighbor_count(pole_cx, pole_cz, ellipsoid) ==
                  HEXGRID_MAX_NEIGHBORS);
        }
    }

    SUBCASE("the pole cell is reached from every meridian, so no band is excluded") {
        int32_t reference_cx = 0;
        int32_t reference_cz = 0;
        hexgrid_geodetic_to_cell(90.0f, 0.0f, ellipsoid, reference_cx, reference_cz);
        bool same_cell = true;
        for (float longitude = -180.0f; longitude < 180.0f; longitude += 45.0f) {
            int32_t cx = 0;
            int32_t cz = 0;
            hexgrid_geodetic_to_cell(90.0f, longitude, ellipsoid, cx, cz);
            if (cx != reference_cx || cz != reference_cz) {
                same_cell = false;
            }
        }
        CHECK(same_cell);
    }
}

TEST_CASE("Hexgrid - the round trip cell to geodetic to cell is the identity") {
    const HexgridEllipsoid ellipsoid = sphere_for_level(TEST_LEVEL);
    CellTable table{};
    fill_cell_table(TEST_LEVEL, table);
    REQUIRE(table.count == static_cast<size_t>(expected_cell_count(TEST_LEVEL)));

    uint32_t identical = 0u;
    uint32_t pentagons_checked = 0u;
    bool inside_range = true;
    for (size_t t = 0u; t < table.count; ++t) {
        float latitude = 0.0f;
        float longitude = 0.0f;
        hexgrid_cell_to_geodetic(table.cx[t], table.cz[t], ellipsoid, latitude, longitude);
        if (latitude < -90.0f || latitude > 90.0f || longitude < -180.0f || longitude > 180.0f) {
            inside_range = false;
        }
        int32_t cx = 0;
        int32_t cz = 0;
        hexgrid_geodetic_to_cell(latitude, longitude, ellipsoid, cx, cz);
        if (cx == table.cx[t] && cz == table.cz[t]) {
            ++identical;
        }
        if (hexgrid_cell_is_pentagon(table.cx[t], table.cz[t], ellipsoid)) {
            ++pentagons_checked;
        }
    }
    CHECK(inside_range);
    CHECK(identical == table.count);
    CHECK(pentagons_checked == HEXGRID_PENTAGON_COUNT);

    SUBCASE("the pole cells round trip as well, they are not a special case") {
        for (float pole_latitude = -90.0f; pole_latitude < 91.0f; pole_latitude += 180.0f) {
            int32_t cx = 0;
            int32_t cz = 0;
            hexgrid_geodetic_to_cell(pole_latitude, 0.0f, ellipsoid, cx, cz);
            float latitude = 0.0f;
            float longitude = 0.0f;
            hexgrid_cell_to_geodetic(cx, cz, ellipsoid, latitude, longitude);
            int32_t back_cx = 0;
            int32_t back_cz = 0;
            hexgrid_geodetic_to_cell(latitude, longitude, ellipsoid, back_cx, back_cz);
            CHECK(back_cx == cx);
            CHECK(back_cz == cz);
        }
    }
}

TEST_CASE("Hexgrid - growth is monotone, address stable, and adds a closed ring") {
    const HexgridEllipsoid before = sphere_for_level(GROWTH_LEVEL);
    const HexgridEllipsoid after = sphere_for_level(TEST_LEVEL);

    SUBCASE("the cell count follows the surface and never shrinks") {
        uint64_t previous = 0u;
        bool monotone = true;
        bool matches_topology = true;
        for (uint32_t level = 0u; level <= 6u; ++level) {
            const uint64_t count =
                hexgrid_cell_count(sphere_for_level(level), HEXGRID_CELL_AREA_M2);
            if (count != expected_cell_count(level)) {
                matches_topology = false;
            }
            if (level > 0u && count <= previous) {
                monotone = false;
            }
            previous = count;
        }
        CHECK(monotone);
        CHECK(matches_topology);
    }

    SUBCASE("an epoch adds exactly the difference and a still radius adds nothing") {
        CHECK(hexgrid_epoch_cell_increment(before, after, HEXGRID_CELL_AREA_M2) ==
              expected_cell_count(TEST_LEVEL) - expected_cell_count(GROWTH_LEVEL));
        CHECK(hexgrid_epoch_cell_increment(before, before, HEXGRID_CELL_AREA_M2) == 0u);
        CHECK(hexgrid_epoch_cell_increment(after, before, HEXGRID_CELL_AREA_M2) == 0u);
    }

    SUBCASE("every cell of the old epoch keeps its address and its anchor bit for bit") {
        SmallCellTable old_table{};
        fill_small_table(GROWTH_LEVEL, old_table);
        REQUIRE(old_table.count == static_cast<size_t>(expected_cell_count(GROWTH_LEVEL)));

        uint32_t stable = 0u;
        for (size_t t = 0u; t < old_table.count; ++t) {
            float before_lat = 0.0f;
            float before_lon = 0.0f;
            float after_lat = 0.0f;
            float after_lon = 0.0f;
            hexgrid_cell_to_geodetic(old_table.cx[t], old_table.cz[t], before, before_lat,
                                     before_lon);
            hexgrid_cell_to_geodetic(old_table.cx[t], old_table.cz[t], after, after_lat, after_lon);
            const bool still_a_cell =
                hexgrid_cell_neighbor_count(old_table.cx[t], old_table.cz[t], after) > 0u;
            if (still_a_cell && before_lat == after_lat && before_lon == after_lon) {
                ++stable;
            }
        }
        CHECK(stable == old_table.count);
    }

    SUBCASE("the growth adds no man's land as a closed ring around every old cell") {
        SmallCellTable old_table{};
        fill_small_table(GROWTH_LEVEL, old_table);
        REQUIRE(old_table.count == static_cast<size_t>(expected_cell_count(GROWTH_LEVEL)));

        uint32_t ringed = 0u;
        for (size_t t = 0u; t < old_table.count; ++t) {
            int32_t neighbor_cx[HEXGRID_MAX_NEIGHBORS] = {};
            int32_t neighbor_cz[HEXGRID_MAX_NEIGHBORS] = {};
            const uint32_t written =
                hexgrid_cell_neighbors(old_table.cx[t], old_table.cz[t], after, neighbor_cx,
                                       neighbor_cz, HEXGRID_MAX_NEIGHBORS);
            const uint32_t expected =
                hexgrid_cell_neighbor_count(old_table.cx[t], old_table.cz[t], before);
            bool all_new = written == expected;
            for (uint32_t a = 0u; a < written; ++a) {
                for (size_t o = 0u; o < old_table.count; ++o) {
                    if (neighbor_cx[a] == old_table.cx[o] && neighbor_cz[a] == old_table.cz[o]) {
                        all_new = false;
                    }
                }
            }
            if (all_new) {
                ++ringed;
            }
        }
        CHECK(ringed == old_table.count);
    }
}
