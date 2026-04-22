cmake_minimum_required(VERSION 3.22)
include(FetchContent)
include(CMakeDependentOption)

project(corephys
	VERSION 3.2.0
	DESCRIPTION "CorePhys, a Jimokomi-focused 2D physics engine based on Box2D"
	HOMEPAGE_URL "https://github.com/devl0rd/corephys"
	LANGUAGES C CXX
)

# stuff to help debug cmake
# message(STATUS "cmake tool chain: ${CMAKE_TOOLCHAIN_FILE}")
# message(STATUS "cmake source dir: ${CMAKE_SOURCE_DIR}")
# message(STATUS "library postfix: ${CMAKE_DEBUG_POSTFIX}")
message(STATUS "CMake C compiler: ${CMAKE_C_COMPILER_ID}")
message(STATUS "CMake C++ compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMake system name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMake host system processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")

# static link to make leak checking easier (_crtBreakAlloc)
# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if (MSVC OR APPLE)
	option(COREPHYS_SANITIZE "Enable sanitizers for some builds" OFF)
	if(COREPHYS_SANITIZE)
		message(STATUS "CorePhys Sanitize")
		# sanitizers need to apply to all compiled libraries to work well
		if(MSVC)
			# address sanitizer only in the debug build
			add_compile_options("$<$<CONFIG:Debug>:/fsanitize=address>")
			add_link_options("$<$<CONFIG:Debug>:/INCREMENTAL:NO>")
		elseif(APPLE)
			add_compile_options(-fsanitize=address -fsanitize-address-use-after-scope -fsanitize=undefined)
			add_link_options(-fsanitize=address -fsanitize-address-use-after-scope -fsanitize=undefined)
		endif()
	else()
		if(MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND PROJECT_IS_TOP_LEVEL)
			# enable hot reloading
			add_compile_options("$<$<CONFIG:Debug>:/ZI>")
			add_link_options("$<$<CONFIG:Debug>:/INCREMENTAL>")
		endif()
	endif()
endif()

# Deterministic math
# https://box2d.org/posts/2024/08/determinism/
if (MINGW OR APPLE OR UNIX)
	add_compile_options(-ffp-contract=off)
endif()

option(COREPHYS_DISABLE_SIMD "Disable SIMD math (slower)" OFF)
option(COREPHYS_COMPILE_WARNING_AS_ERROR "Compile warnings as errors" OFF)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
	cmake_dependent_option(COREPHYS_AVX2 "Enable AVX2" OFF "NOT COREPHYS_DISABLE_SIMD" OFF)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Needed for samples.exe to find the CorePhys DLL
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

add_subdirectory(src)

# This hides samples, test, and doxygen from apps that use CorePhys via FetchContent
if(PROJECT_IS_TOP_LEVEL)
	option(COREPHYS_SAMPLES "Build the CorePhys samples" ON)
	option(COREPHYS_BENCHMARKS "Build the CorePhys benchmarks" OFF)
	option(COREPHYS_DOCS "Build the CorePhys documentation" OFF)
	option(COREPHYS_PROFILE "Enable profiling with Tracy" OFF)
	option(COREPHYS_VALIDATE "Enable heavy validation" ON)
	option(COREPHYS_UNIT_TESTS "Build the CorePhys unit tests" ON)
	
	if(COREPHYS_UNIT_TESTS OR COREPHYS_SAMPLES OR COREPHYS_BENCHMARKS)
		# Emscripten pthread support for enkiTS
		if(EMSCRIPTEN)
			set(EMSCRIPTEN_PTHREADS_COMPILER_FLAGS "-pthread -s USE_PTHREADS=1")
			set(EMSCRIPTEN_PTHREADS_LINKER_FLAGS "${EMSCRIPTEN_PTHREADS_COMPILER_FLAGS} -s ALLOW_MEMORY_GROWTH")
			string(APPEND CMAKE_C_FLAGS " ${EMSCRIPTEN_PTHREADS_COMPILER_FLAGS}")
			string(APPEND CMAKE_CXX_FLAGS " ${EMSCRIPTEN_PTHREADS_COMPILER_FLAGS}")
			string(APPEND CMAKE_EXE_LINKER_FLAGS " ${EMSCRIPTEN_PTHREADS_LINKER_FLAGS}")
		endif()

		# Task system used in tests and samples
		set(ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "Build enkiTS examples")
		FetchContent_Declare(
			enkits
			GIT_REPOSITORY https://github.com/dougbinks/enkiTS.git
			GIT_TAG master
			GIT_SHALLOW TRUE
			GIT_PROGRESS TRUE
		)
		FetchContent_MakeAvailable(enkits)

		add_subdirectory(shared)
	endif()

	# Tests need static linkage because they test internal CorePhys functions
	if(NOT BUILD_SHARED_LIBS AND COREPHYS_UNIT_TESTS)
		message(STATUS "Adding CorePhys unit tests")
		add_subdirectory(test)
		set_target_properties(test PROPERTIES XCODE_GENERATE_SCHEME TRUE)
	else()
		message(STATUS "Skipping CorePhys unit tests")
	endif()

	if(COREPHYS_SAMPLES)
		add_subdirectory(samples)

		# default startup project for Visual Studio
		if(MSVC)
			set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT samples)
			set_property(TARGET samples PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
		endif()

		if(APPLE)
			set_target_properties(samples PROPERTIES
				XCODE_GENERATE_SCHEME TRUE
				XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
		endif()
	endif()

	if(COREPHYS_BENCHMARKS)
		add_subdirectory(benchmark)
		set_target_properties(benchmark PROPERTIES XCODE_GENERATE_SCHEME TRUE)
	endif()

	if(COREPHYS_DOCS)
		add_subdirectory(docs)
	endif()
endif()

# # Building on clang in windows
# cmake -S .. -B . -G "Visual Studio 17 2022" -A x64 -T ClangCL
# https://clang.llvm.org/docs/UsersManual.html#clang-cl
