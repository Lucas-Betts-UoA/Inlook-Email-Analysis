cmake_minimum_required(VERSION 3.20)
project(Inlook LANGUAGES CXX VERSION 1.0)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
elseif (WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /WHOLEARCHIVE")
endif()

add_compile_options(-Wno-unknown-attributes)
add_link_options(-Wno-unused-command-line-argument)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(COMMON_COMPILE_OPTIONS -Wall -Wextra -pedantic -fPIC)
elseif (MSVC)
    set(COMMON_COMPILE_OPTIONS /W4)
endif()

# Gather all source files for Inlook_Core
file(GLOB SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")

# Create a shared library for core functionality
add_library(Inlook_Core SHARED ${SRC_FILES})

# Ensure Inlook_Core exports
target_compile_options(Inlook_Core PRIVATE -fvisibility=default)
target_link_options(Inlook_Core PRIVATE -W)

# Include external dependencies
add_subdirectory(external)

# Include directories
target_include_directories(Inlook_Core PUBLIC "include")

# Define PLUGIN_TARGETS
set(PLUGIN_TARGETS "" CACHE INTERNAL "List of plugin targets")
add_subdirectory(plugins)

# Add main executable
add_executable(Inlook src/Main.cpp)  # Main executable requires entry file

# Link against the **static** library and force exports
target_link_libraries(Inlook PRIVATE Inlook_Core)

# Set output directory for the main application
set_target_properties(Inlook PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Ensure plugins link properly
foreach(plugin ${PLUGIN_TARGETS})
    target_link_libraries(${plugin} PRIVATE Inlook_Core)
    target_compile_options(${plugin} PRIVATE -fvisibility=default)
endforeach()

# Copy configuration files
set(CONFIG_SOURCE_DIR "${CMAKE_SOURCE_DIR}/config/")
set(CONFIG_DEST_DIR "${CMAKE_BINARY_DIR}/")
add_custom_target(copy_configs ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CONFIG_SOURCE_DIR} ${CONFIG_DEST_DIR}
        COMMENT "Copying ./config/ to the build directory"
)

# Copy web files
set(HTML_TEMPLATES_SOURCE_DIR "${CMAKE_SOURCE_DIR}/web")
set(HTML_TEMPLATES_DESTINATION_DIR "${CMAKE_BINARY_DIR}/web")
add_custom_target(copy_html_templates ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${HTML_TEMPLATES_SOURCE_DIR} ${HTML_TEMPLATES_DESTINATION_DIR}
        COMMENT "Copying ./web/ to the build directory"
)

# Copy docs directory if it exists
set(DOCS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/docs")
set(DOCS_DEST_DIR "${CMAKE_BINARY_DIR}/web/docs")

if(EXISTS ${DOCS_SOURCE_DIR})
    add_custom_target(copy_docs ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${DOCS_SOURCE_DIR} ${DOCS_DEST_DIR}
            COMMENT "Copying ./docs/ to the web directory"
    )
    add_dependencies(Inlook copy_docs)
endif()

# Copy README.md files if they exist
file(GLOB README_FILES "${CMAKE_SOURCE_DIR}/README.md")
if(README_FILES)
    add_custom_target(copy_readme ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${README_FILES} ${CMAKE_BINARY_DIR}
            COMMENT "Copying README.md files to the build directory"
    )
    add_dependencies(Inlook copy_readme)
endif()

add_dependencies(Inlook copy_html_templates copy_configs ${PLUGIN_TARGETS})