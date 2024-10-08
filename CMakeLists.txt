cmake_minimum_required(VERSION 3.30)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

project(groad VERSION 0.0.1 LANGUAGES CXX)

if (NOT WIN32)
    message(FATAL_ERROR "Only Windows platform is supported!")
endif()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(WARNING "Config was tested only on MVSC compiler!")
endif()

if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "19.35.32216.1")
    message(WARNING
        "Config was tested on the MSVC compiler version 19.35.32216.1\n"
        "Your compiler version: ${CMAKE_CXX_COMPILER_VERSION}"
    )
endif()

# cmake .. -T version=14.41.17.11
if (DEFINED CMAKE_VS_PLATFORM_TOOLSET_VERSION AND CMAKE_VS_PLATFORM_TOOLSET_VERSION VERSION_LESS "14.41.17.11")
    message(WARNING "The toolset version does not support C++20 Modules")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release" CACHE STRING "Available configs" FORCE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(EXTERNAL_DIR external)
get_filename_component(EXTERNAL_DIR ${EXTERNAL_DIR} ABSOLUTE)
set(EXTERNAL_DIR ${EXTERNAL_DIR}/)

set(ASSETS_DIR assets)
get_filename_component(ASSETS_DIR ${ASSETS_DIR} ABSOLUTE)
set(ASSETS_DIR ${ASSETS_DIR}/)

add_subdirectory(${EXTERNAL_DIR})

add_subdirectory(src)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CMAKE_PROJECT_NAME})

set_property(TARGET ImGui PROPERTY FOLDER "ThirdPartyLibraries")

set_property(TARGET DirectXTK PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET Continuous PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET Experimental PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET Nightly PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET NightlyMemoryCheck PROPERTY FOLDER "ThirdPartyLibraries")
set_property(TARGET xwbtool PROPERTY FOLDER "ThirdPartyLibraries")

set_property(TARGET MGSandbox PROPERTY FOLDER "University")
set_property(TARGET MGSerializer PROPERTY FOLDER "University")

set_property(TARGET copy_assets PROPERTY FOLDER "Tools")