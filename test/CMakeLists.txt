# CorePhys unit test app

set(COREPHYS_TEST_FILES
    main.c
    test_bitset.c
    test_collision.c
    test_container.c
    test_determinism.c
    test_distance.c
    test_dynamic_tree.c
    test_id.c
    test_macros.h
    test_math.c
    test_particle.c
    test_particle_helpers.c
    test_particle_parallel.c
    test_particle_scenarios.c
    test_shape.c
    test_table.c
    test_world.c
)

add_executable(test ${COREPHYS_TEST_FILES})

set_target_properties(test PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)

if(COREPHYS_COMPILE_WARNING_AS_ERROR)
    set_target_properties(test PROPERTIES COMPILE_WARNING_AS_ERROR ON)
endif()

# Special access to CorePhys internals for testing
target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(test PRIVATE corephys shared enkiTS)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "" FILES ${COREPHYS_TEST_FILES})
