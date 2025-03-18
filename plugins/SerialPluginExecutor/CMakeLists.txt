project(SerialPluginExecutor LANGUAGES CXX VERSION 1.0)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add the plugin as a shared library
file(GLOB SRC_FILES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
include_directories(include)
add_library(SerialPluginExecutor SHARED ${SRC_FILES})

# Ensure position-independent code (best practice for shared libraries)
set_target_properties(SerialPluginExecutor PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(SerialPluginExecutor PRIVATE -fvisibility=default)

# Set the output directory for the plugin
set_target_properties(SerialPluginExecutor PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/${PROJECT_NAME}
)
message(STATUS "LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/${PROJECT_NAME}/")