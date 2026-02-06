cmake_minimum_required(VERSION 3.16)
project(sudoku_demo CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the main project as a subdirectory to get the nd-wfc library
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../src ${CMAKE_CURRENT_BINARY_DIR}/nd-wfc)

# Enable testing
enable_testing()

# Add compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Use FetchContent to get Google Test for consistent builds
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)
set(HAS_GTEST TRUE)
message(STATUS "Using Google Test via FetchContent")

# Find Google Benchmark (optional)
find_package(benchmark)
if(benchmark_FOUND)
    set(HAS_BENCHMARK TRUE)
else()
    set(HAS_BENCHMARK FALSE)
    message(WARNING "Google Benchmark not found. Benchmarks will not be built.")
endif()

# Find threading library for platform-specific threading support
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)

# Ensure consistent runtime library settings for MSVC
if(MSVC)
    # Force all targets to use the same runtime library
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    # Ensure Google Test uses the same runtime library
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()



# Create WFC demo executable
add_executable(sudoku_wfc_demo
    sudoku_wfc.cpp
    sudoku.cpp
)





# Link executable to the nd-wfc library
target_link_libraries(sudoku_wfc_demo PRIVATE nd-wfc)

# Ensure consistent runtime library settings for executable
if(MSVC)
    target_compile_options(sudoku_wfc_demo PRIVATE $<$<CONFIG:Debug>:/MDd> $<$<CONFIG:Release>:/MD>)
endif()



# Set output directory and properties for sudoku_wfc_demo
set_target_properties(sudoku_wfc_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)





# Include directories (current source directory for local headers)
target_include_directories(sudoku_wfc_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Optional: Enable optimizations for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(sudoku_wfc_demo PRIVATE /O2)
    else()
        target_compile_options(sudoku_wfc_demo PRIVATE -O3 -march=native)
    endif()
endif()

# Create test executable (if GTest is available)
if(HAS_GTEST)
    add_executable(sudoku_tests
        sudoku.cpp
        test_sudoku.cpp
    )

    target_link_libraries(sudoku_tests gtest gtest_main nd-wfc)

    # Ensure consistent runtime library settings for test executable
    if(MSVC)
        target_compile_options(sudoku_tests PRIVATE $<$<CONFIG:Debug>:/MDd> $<$<CONFIG:Release>:/MD>)
        # Ensure Google Test targets use consistent runtime library
        set_target_properties(gtest gtest_main gmock gmock_main PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
        )
    endif()
    if(Threads_FOUND)
        target_link_libraries(sudoku_tests Threads::Threads)
    endif()

    # Set test output directory
    set_target_properties(sudoku_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )

    # Include directories for tests
    target_include_directories(sudoku_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # Add test to CTest
    add_test(NAME sudoku_tests COMMAND sudoku_tests)
endif()

# Create benchmark executable (if Google Benchmark is available)
if(HAS_BENCHMARK)
    add_executable(sudoku_benchmarks
        sudoku.cpp
        benchmark_sudoku.cpp
    )

    target_link_libraries(sudoku_benchmarks benchmark::benchmark nd-wfc)
    if(Threads_FOUND)
        target_link_libraries(sudoku_benchmarks Threads::Threads)
    endif()

    # Set benchmark output directory
    set_target_properties(sudoku_benchmarks PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )

    # Include directories for benchmarks
    target_include_directories(sudoku_benchmarks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# Installation (optional)
if(HAS_GTEST AND HAS_BENCHMARK)
    install(TARGETS sudoku_wfc_demo sudoku_tests sudoku_benchmarks DESTINATION bin)
elseif(HAS_GTEST)
    install(TARGETS sudoku_wfc_demo sudoku_tests DESTINATION bin)
else()
    install(TARGETS sudoku_wfc_demo DESTINATION bin)
endif()
