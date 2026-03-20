option(ASSIMP_BUILD_TESTS OFF)
option(ASSIMP_INSTALL OFF)
add_subdirectory(ImGui)
# TODO: remove DXTK
add_subdirectory(DirectXTK)
# University requirement
add_subdirectory(Scene-Serializer/cpp/Serializer)

if(GROAD_ENABLE_CEF AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cef/CMakeLists.txt")
    set(USE_SANDBOX OFF CACHE BOOL "Disable the CEF sandbox for groad." FORCE)
    set(CEF_RUNTIME_LIBRARY_FLAG "/MD" CACHE STRING "Use the dynamic MSVC runtime for the CEF wrapper." FORCE)

    set(CEF_VERSION "144.0.6+g5f7e671+chromium-144.0.7559.59")
    set(CEF_CHANNEL "")

    if(WIN32)
        if("${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64")
            set(CEF_PLATFORM "windowsarm64")
        elseif(CMAKE_SIZEOF_VOID_P MATCHES 8)
            set(CEF_PLATFORM "windows64")
        else()
            set(CEF_PLATFORM "windows32")
        endif()
    else()
        message(FATAL_ERROR "CEF integration is currently only wired for Windows.")
    endif()

    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cef/cmake")
    include(DownloadCEF)
    DownloadCEF("${CEF_PLATFORM}" "${CEF_CHANNEL}" "${CEF_VERSION}" "${CMAKE_CURRENT_SOURCE_DIR}/cef/third_party/cef")

    list(APPEND CMAKE_MODULE_PATH "${CEF_ROOT}/cmake")
    find_package(CEF REQUIRED)
    ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
    add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} cef_libcef_dll_wrapper)

    add_library(GroadCEF INTERFACE)
    add_library(Groad::CEF ALIAS GroadCEF)
    target_include_directories(GroadCEF INTERFACE "${CEF_ROOT}")
    target_link_libraries(GroadCEF INTERFACE libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS})
    target_compile_definitions(GroadCEF INTERFACE GROAD_WITH_CEF=1)

    set(GROAD_WITH_CEF ON PARENT_SCOPE)
    set(GROAD_CEF_BINARY_DIR "${CEF_BINARY_DIR}" PARENT_SCOPE)
    set(GROAD_CEF_RESOURCE_DIR "${CEF_RESOURCE_DIR}" PARENT_SCOPE)
    set(GROAD_CEF_BINARY_FILES "${CEF_BINARY_FILES}" PARENT_SCOPE)
else()
    if(GROAD_ENABLE_CEF)
        message(WARNING "GROAD_ENABLE_CEF is ON, but external/cef is missing. Building without CEF support.")
    endif()

    set(GROAD_WITH_CEF OFF PARENT_SCOPE)
endif()
