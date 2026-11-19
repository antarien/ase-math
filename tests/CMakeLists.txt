#[[
Copyright (c) Jan Ohlmann. All rights reserved.
Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
SPDX-License-Identifier: LicenseRef-ASE-LIC

usecase:101396 - assure engine component quality, actor Engine Unit Test (ENGUTEST).
The acceptance of step T.1 of PLAN_ASE_VIVARIUM_TREEGEN: perlin3, fbm3_vec, hermite,
segment_distance_squared, douglas_peucker and mul.

THIS FILE WAS REMOVED BY 50efcf9 (2026-09-16) AND STANDS AGAIN ON PURPOSE. That commit took the
hex grid arithmetic out of ase-math - a grid belongs to ase-gis - and with it hexgrid_test.cpp,
the only test this directory then held; the file went because nothing was left to build, not
because ase-math was to carry no tests. The hook if(ASE_BUILD_TESTS AND EXISTS tests) in
../CMakeLists.txt stayed in that commit and waits for this directory.
]]

add_executable(ase-math-test
    growth_math_test.cpp
)

target_link_libraries(ase-math-test
    PRIVATE
        ase::math
        doctest::doctest
)

# The define is a property of the TARGET: it decides which translation unit receives main(), and
# this target has exactly one.
target_compile_definitions(ase-math-test PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)

add_test(NAME ase-math-test COMMAND ase-math-test)

set_target_properties(ase-math-test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
