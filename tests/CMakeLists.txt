include(FetchContent)

# Fetch Google Test
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

set(TEST_SOURCES
    test_main.cpp
    test_allocator.cpp
)

# Create test executable
add_executable(nd-wfc-tests ${TEST_SOURCES})

# Include directories
target_include_directories(nd-wfc-tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Link libraries
target_link_libraries(nd-wfc-tests
    PRIVATE
        nd-wfc
        gtest_main
        gtest
)

# Set C++ standard
set_target_properties(nd-wfc-tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(nd-wfc-tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTIES
        TIMEOUT 60
)

# Add test target to run all tests
add_custom_target(run-tests
    COMMAND nd-wfc-tests
    DEPENDS nd-wfc-tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
