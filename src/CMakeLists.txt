# Create interface library (header-only)
add_library(nd-wfc INTERFACE)

# Create main executable
add_executable(nd-wfc-main main.cpp)

# Include directories for interface library
target_include_directories(nd-wfc
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Include directories for main executable
target_include_directories(nd-wfc-main
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Link external dependencies
# These libraries are available but not currently used in the core library
# They will be used for future features like graphics, model loading, etc.

# RapidJSON - header-only, no linking needed
if(TARGET rapidjson)
    target_link_libraries(nd-wfc INTERFACE rapidjson)
endif()

# Assimp - for 3D model loading (future use)
if(TARGET assimp)
    target_link_libraries(nd-wfc INTERFACE assimp)
endif()

# SDL3 - for graphics and windowing (future use)
if(TARGET SDL3::SDL3)
    target_link_libraries(nd-wfc INTERFACE SDL3::SDL3)
endif()

# SDL_image - for image loading (future use) - temporarily disabled
# if(TARGET SDL3_image::SDL3_image)
#     target_link_libraries(nd-wfc INTERFACE SDL3_image::SDL3_image)
# endif()

# Set properties
set_target_properties(nd-wfc PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "nd-wfc"
)

# Install configuration temporarily disabled
# TODO: Fix install configuration
# install(TARGETS nd-wfc
#     EXPORT nd-wfc-targets
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
# )

# Install headers
# install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/
#     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#     FILES_MATCHING PATTERN "*.hpp"
# )
