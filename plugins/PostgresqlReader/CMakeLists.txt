project(PostgresqlReader LANGUAGES CXX)

# Enable verbose output during the build process (optional)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Add the plugin as a shared library
file(GLOB SRC_FILES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
include_directories(include)
add_library(PostgresqlReader SHARED ${SRC_FILES})

# Ensure position-independent code (best practice for shared libraries)
set_target_properties(PostgresqlReader PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Set the output directory for the plugin
set_target_properties(PostgresqlReader PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/${PROJECT_NAME}
)
