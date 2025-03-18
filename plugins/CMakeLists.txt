# Find all plugins and add them as subdirectories
file(GLOB PLUGIN_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/*")
message(STATUS "Working through plugins directories.")
foreach(PLUGIN_DIR ${PLUGIN_DIRS})
    if(IS_DIRECTORY ${PLUGIN_DIR})
        message(STATUS "PLUGIN_DIR: ${PLUGIN_DIR}")
        get_filename_component(PLUGIN_NAME ${PLUGIN_DIR} NAME)
        add_subdirectory(${PLUGIN_DIR})
        list(APPEND PLUGIN_TARGETS ${PLUGIN_NAME})
    endif()
endforeach()

set(PLUGIN_TARGETS ${PLUGIN_TARGETS} PARENT_SCOPE)
message(STATUS "Plugin targets: ${PLUGIN_TARGETS}")
