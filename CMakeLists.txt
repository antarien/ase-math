cmake_minimum_required(VERSION 3.20)

#[[
ASE FOUNDATION BUILD CONFIGURATION

@file        CMakeLists.txt
@brief       Build configuration for ase-math
@description Math library - Vectors, matrices, noise functions, interpolation.
             Header-only library for mathematical operations.

@module      ase-math
@layer       0 (Foundation)
@created     2025-12-15
@modified    2026-01-30
@version     00.00.11.00011 [poc]

CMAKE MODULE COMPLIANCE

[ ] cmake_minimum_required(VERSION 3.20) as FIRST line
[ ] project() with VERSION and LANGUAGES CXX
[ ] message(STATUS) for configuration output
[ ] add_library() with INTERFACE for header-only
[ ] add_library(ase::{module} ALIAS ...) for namespace
[ ] target_include_directories() with INTERFACE and generator expressions
[ ] target_compile_features() with cxx_std_20
[ ] NO external dependencies (Layer 0)
[ ] include(GNUInstallDirs) for installation
[ ] install(DIRECTORY include/ ...) for headers
]]

# Parse local VERSION (SSOT) if not building from root
if(NOT DEFINED ASE_MATH_VERSION)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
        message(FATAL_ERROR "VERSION file not found: ${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    endif()
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _VERSION_LINES)
    foreach(_LINE ${_VERSION_LINES})
        if(_LINE MATCHES "^ASE_MATH=(.*)$")
            set(ASE_MATH_VERSION "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    if(NOT DEFINED ASE_MATH_VERSION)
        message(FATAL_ERROR "ASE_MATH version not found in VERSION file")
    endif()
endif()

project(ase-math VERSION ${ASE_MATH_VERSION} LANGUAGES CXX)

# Generate version.hpp from template (SSOT)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/ase/math/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/ase/math/version.hpp"
    @ONLY
)

# ==============================================================================
# BUILD CONFIGURATION
# ==============================================================================



message(STATUS "Configuring ${PROJECT_NAME} v${PROJECT_VERSION}")

#[[
HEADER-ONLY LIBRARY
Mathematical functions: Vec2, Vec3, Mat4, noise, lerp, clamp, etc.
]]
add_library(${PROJECT_NAME} INTERFACE
    #[[
    CORE
    Main mathematical functions and entry point.
    ]]
    include/ase/math/math.hpp

    #[[
    VECTORS
    Vector types for 2D and 3D operations.
    ]]
    include/ase/math/vec3.hpp

    #[[
    COLORS
    Color types and operations.
    ]]
    include/ase/math/color3.hpp

    #[[
    QUATERNION
    Quaternion type for rotations.
    ]]
    include/ase/math/quaternion.hpp

    #[[
    GEOMETRY
    Geometric primitives and operations.
    ]]
    include/ase/math/geometry.hpp
    include/ase/math/polygon.hpp
    include/ase/math/spherical.hpp
    include/ase/math/hexgrid.hpp

    #[[
    INTERPOLATION
    Interpolation functions (lerp, slerp, etc.).
    ]]
    include/ase/math/interpolation.hpp

    #[[
    NOISE
    Noise generation functions.
    ]]
    include/ase/math/perlin.hpp

    #[[
    RANDOM
    Random number generation utilities.
    ]]
    include/ase/math/random.hpp
    include/ase/math/random_std.hpp

    #[[
    SCALARS
    Scalar helpers the vector and interpolation headers build on.
    ]]
    include/ase/math/scalar.hpp
)
add_library(ase::math ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

#[[ VERBOTEN: No dependencies allowed in Layer 0! ]]

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(ASE_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    add_subdirectory(tests)
endif()
