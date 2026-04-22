# CorePhys code shared by samples, benchmarks, and unit tests

set(COREPHYS_SHARED_FILES
	benchmarks.c
	benchmarks.h
	determinism.c
	determinism.h
	human.c
	human.h
	particle_parallel_benchmarks.c
	particle_parallel_benchmarks.h
	particle_scenarios.c
	particle_scenarios.h
	random.c
	random.h
)

add_library(shared STATIC ${COREPHYS_SHARED_FILES})

set_target_properties(shared PROPERTIES
	C_STANDARD 17
)

if (COREPHYS_COMPILE_WARNING_AS_ERROR)
	set_target_properties(shared PROPERTIES COMPILE_WARNING_AS_ERROR ON)
endif()

target_include_directories(shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(shared PRIVATE corephys)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "" FILES ${COREPHYS_SHARED_FILES})
