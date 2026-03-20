cmake_minimum_required(VERSION 3.30)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

if(POLICY CMP0155)
    cmake_policy(SET CMP0155 NEW)
endif()

project(groad VERSION 0.0.1 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Only Windows platform is supported!")
endif()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(WARNING "Config was tested only on the MSVC compiler.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# cmake .. -T version=19.44.35217.0
set(TESTED_COMPILER_VERSION "19.44.35217.0")
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL ${TESTED_COMPILER_VERSION})
    message(WARNING
        "Config was tested on the MSVC compiler version ${TESTED_COMPILER_VERSION}\n"
        "Your compiler version: ${CMAKE_CXX_COMPILER_VERSION}"
    )
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release" CACHE STRING "Available configs" FORCE)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external")
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")

option(GROAD_USE_EXTERNAL_SUBMODULES "Use real dependencies from the external/ submodule directory when available." ON)
option(GROAD_ENABLE_CEF "Enable real CEF integration when the external/cef submodule is available." ON)
set(GROAD_USING_STUBS OFF)

if(GROAD_USE_EXTERNAL_SUBMODULES AND EXISTS "${EXTERNAL_DIR}/CMakeLists.txt")
    add_subdirectory("${EXTERNAL_DIR}")
else()
    set(GROAD_USING_STUBS ON)
    message(WARNING
        "external/ was not found, so groad will configure in IDE-friendly stub mode.\n"
        "This keeps CMake + IntelliSense working from the ZIP archive.\n"
        "To build the real application, clone with submodules or run: git submodule update --init --recursive"
    )

    add_library(ImGui INTERFACE)
    add_library(ImGui::ImGui ALIAS ImGui)
    target_include_directories(ImGui INTERFACE "${CMAKE_SOURCE_DIR}/stubs")

    add_library(DirectXTK INTERFACE)
    target_include_directories(DirectXTK INTERFACE "${CMAKE_SOURCE_DIR}/stubs")

    add_library(MGSerializer INTERFACE)
    target_include_directories(MGSerializer INTERFACE "${CMAKE_SOURCE_DIR}/stubs")
endif()

add_subdirectory(src)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CMAKE_PROJECT_NAME})

set_property(
    TARGET ImGui DirectXTK Continuous Experimental Nightly NightlyMemoryCheck xwbtool
    PROPERTY FOLDER "ThirdPartyLibraries"
)

set_property(
    TARGET MGSandbox MGSerializer
    PROPERTY FOLDER "University"
)

set_property(
    TARGET copy_assets
    PROPERTY FOLDER "Tools"
)
