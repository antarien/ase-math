cmake_minimum_required(VERSION 3.20)

project(ase-math
    VERSION 0.1.0
    DESCRIPTION "ASE Math Library - Vectors, Matrices, Noise"
    LANGUAGES CXX
)

# Library
add_library(ase-math INTERFACE)
add_library(ase::math ALIAS ase-math)

target_include_directories(ase-math
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(ase-math INTERFACE cxx_std_20)

# Install
install(TARGETS ase-math
    EXPORT ase-math-targets
)

install(DIRECTORY include/ase
    DESTINATION include
)
