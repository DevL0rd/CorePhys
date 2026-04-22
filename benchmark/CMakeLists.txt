# CorePhys benchmark app

set(COREPHYS_BENCHMARK_FILES
	main.c
)
add_executable(benchmark ${COREPHYS_BENCHMARK_FILES})

set_target_properties(benchmark PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)

if (COREPHYS_COMPILE_WARNING_AS_ERROR)
	set_target_properties(benchmark PROPERTIES COMPILE_WARNING_AS_ERROR ON)
endif()

target_link_libraries(benchmark PRIVATE corephys shared enkiTS)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "" FILES ${COREPHYS_BENCHMARK_FILES})