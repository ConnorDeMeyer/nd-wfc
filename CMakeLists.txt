cmake_minimum_required(VERSION 3.16)
project(nd-wfc VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure consistent runtime library settings for MSVC
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# Enable testing
enable_testing()

# Options
option(ND_WFC_BUILD_TESTS "Build tests" ON)
option(ND_WFC_BUILD_EXAMPLES "Build examples" ON)
option(ND_WFC_USE_SYSTEM_LIBS "Use system libraries instead of bundled" OFF)

# Add subdirectories
add_subdirectory(src)

if(ND_WFC_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Install configuration temporarily disabled
# TODO: Fix install configuration
# include(GNUInstallDirs)
# install(TARGETS nd-wfc
#     EXPORT nd-wfc-targets
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#     INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
# )

# Export targets
# install(EXPORT nd-wfc-targets
#     FILE nd-wfc-targets.cmake
#     NAMESPACE nd-wfc::
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nd-wfc
# )

# Generate and install package config file
# include(CMakePackageConfigHelpers)
# configure_package_config_file(
#     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/nd-wfc-config.cmake.in"
#         "${CMAKE_CURRENT_BINARY_DIR}/nd-wfc-config.cmake"
#     INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nd-wfc
# )
# install(FILES "${CMAKE_CURRENT_BINARY_DIR}/nd-wfc-config.cmake"
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nd-wfc
# )
