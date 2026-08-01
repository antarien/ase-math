add_executable(ase-math-test
    hexgrid_test.cpp
)

#[[
The doctest entry point is switched on here rather than through a define in the test source, so
the test file stays free of macro definitions.
]]
target_compile_definitions(ase-math-test
    PRIVATE
        DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
)

target_link_libraries(ase-math-test
    PRIVATE
        ase::math
        doctest::doctest
)

add_test(NAME ase-math-test COMMAND ase-math-test)

set_target_properties(ase-math-test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
