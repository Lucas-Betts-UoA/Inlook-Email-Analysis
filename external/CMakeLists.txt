# Add each external dependency as a subdirectory
add_subdirectory(json)
add_subdirectory(libpqxx)
add_subdirectory(json-scheme-validator)
add_subdirectory(fastText)

# Handle ICU across platforms
if (APPLE)
    execute_process(COMMAND brew --prefix icu4c OUTPUT_VARIABLE ICU_ROOT OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(ICU_INCLUDE_DIR "${ICU_ROOT}/include")
    set(ICU_LIBRARY_DIR "${ICU_ROOT}/lib")
elseif (UNIX AND NOT APPLE)
    set(ICU_ROOT "/usr")
    set(ICU_INCLUDE_DIR "${ICU_ROOT}/include")
    set(ICU_LIBRARY_DIR "${ICU_ROOT}/lib")
elseif (WIN32)
    if (DEFINED ENV{VCPKG_ROOT})
        set(ICU_ROOT "$ENV{VCPKG_ROOT}/installed/x64-windows")
    else()
        set(ICU_ROOT "C:/icu")
    endif()
    set(ICU_INCLUDE_DIR "${ICU_ROOT}/include")
    set(ICU_LIBRARY_DIR "${ICU_ROOT}/lib")
endif()

find_package(ICU REQUIRED COMPONENTS uc i18n data)

if (ICU_FOUND)
    message(STATUS "ICU Found: ${ICU_VERSION}")
else()
    message(FATAL_ERROR "ICU not found! Install ICU or set ICU_ROOT manually.")
endif()

# Include directories for shared dependencies
target_include_directories(Inlook_Core PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${ICU_INCLUDE_DIR}
        asio-1.30.2/include
        Crow/include
        ${CMAKE_SOURCE_DIR}/external/fastText/src
        libpqxx/include
)

target_link_directories(Inlook_Core PUBLIC ${ICU_LIBRARY_DIR})

# Link dependencies to the core library
target_link_libraries(Inlook_Core
        PUBLIC
        ICU::uc ICU::i18n ICU::data # Core + Plugins need ICU
        nlohmann_json_schema_validator # JSON schema validator needed for configs
        INTERFACE
        nlohmann_json # Header-only, no linking required
        libpqxx::pqxx
        fasttext-static # Only used by plugins, Core does NOT use it
)

# Apply common compile options
target_compile_options(Inlook_Core INTERFACE ${COMMON_COMPILE_OPTIONS})