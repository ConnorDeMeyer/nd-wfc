cmake_minimum_required(VERSION 3.16)
project(2d_dungeon_demo CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the main project as a subdirectory to get the nd-wfc library
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../src ${CMAKE_CURRENT_BINARY_DIR}/nd-wfc)

# Add compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Create demo executable
add_executable(dungeon_demo main.cpp)

# Link to nd-wfc library
target_link_libraries(dungeon_demo PRIVATE nd-wfc)

# Include directories
target_include_directories(dungeon_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Set output directory and properties
set_target_properties(dungeon_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Optional: Enable optimizations for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(dungeon_demo PRIVATE /O2)
    else()
        target_compile_options(dungeon_demo PRIVATE -O3 -march=native)
    endif()
endif()

# Ensure consistent runtime library settings for MSVC
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    target_compile_options(dungeon_demo PRIVATE $<$<CONFIG:Debug>:/MDd> $<$<CONFIG:Release>:/MD>)
endif()
