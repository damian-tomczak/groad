cmake_minimum_required(VERSION 3.20)
project(DX12FrameworkSkeleton LANGUAGES CXX)

# ---- Options ---------------------------------------------------------------
option(ENABLE_RENDERDOC "Enable RenderDoc API integration (F12 capture)" ON)
option(USE_DXC          "Use DirectX Shader Compiler (dxcompiler) instead of D3DCompile" OFF)

# Path hints (edit these in CMake GUI or pass -D on command line)
set(RENDERDOC_INCLUDE "" CACHE PATH "Path to RenderDoc include directory containing renderdoc_app.h")
set(RENDERDOC_DLL     "" CACHE FILEPATH "Optional path to renderdoc.dll to copy next to the exe")
set(D3DX12_INCLUDE     "" CACHE PATH "Optional path to folder containing d3dx12.h (else rely on your SDK setup)")

# ---- Target ---------------------------------------------------------------
add_executable(DX12FrameworkSkeleton WIN32
  Dx12FrameworkSkeleton.cpp
)

set_target_properties(DX12FrameworkSkeleton PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
)

# Unicode & common Win defines
target_compile_definitions(DX12FrameworkSkeleton PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)

# Include dirs
if(D3DX12_INCLUDE)
  target_include_directories(DX12FrameworkSkeleton PRIVATE "${D3DX12_INCLUDE}")
endif()

if(ENABLE_RENDERDOC)
  target_compile_definitions(DX12FrameworkSkeleton PRIVATE ENABLE_RENDERDOC)
  if(RENDERDOC_INCLUDE)
    target_include_directories(DX12FrameworkSkeleton PRIVATE "${RENDERDOC_INCLUDE}")
  endif()
endif()

# ---- Libraries ------------------------------------------------------------
# Core DirectX / Windows libs
if(MSVC)
  target_link_libraries(DX12FrameworkSkeleton PRIVATE d3d12 dxgi dxguid)
else()
  message(FATAL_ERROR "This project is intended for MSVC/Windows.")
endif()

# Shader compiler: either legacy D3DCompile or DXC
if(USE_DXC)
  # Try to link against dxcompiler; adjust if your environment provides a package
  # You can point CMAKE_PREFIX_PATH to a DXC install with dxcompiler.lib
  find_library(DXCOMPILER_LIB NAMES dxcompiler)
  if(DXCOMPILER_LIB)
    target_link_libraries(DX12FrameworkSkeleton PRIVATE "${DXCOMPILER_LIB}")
    target_compile_definitions(DX12FrameworkSkeleton PRIVATE USE_DXC)
  else()
    message(WARNING "dxcompiler.lib not found; falling back to D3DCompile")
    set(USE_DXC OFF)
  endif()
endif()

if(NOT USE_DXC)
  target_link_libraries(DX12FrameworkSkeleton PRIVATE d3dcompiler)
endif()

# ---- Warnings / Debug flags -----------------------------------------------
if(MSVC)
  target_compile_options(DX12FrameworkSkeleton PRIVATE /W4 /permissive- /Zc:__cplusplus)
  # Generate useful debug info
  string(TOUPPER "${CMAKE_BUILD_TYPE}" _cfg)
  if(_cfg STREQUAL "DEBUG")
    target_compile_options(DX12FrameworkSkeleton PRIVATE /Zi)
    target_link_options(DX12FrameworkSkeleton PRIVATE /DEBUG:FULL)
  endif()
endif()

# ---- Post-build helpers ----------------------------------------------------
# Optionally copy renderdoc.dll next to the exe to enable API capture without injection
if(ENABLE_RENDERDOC AND RENDERDOC_DLL AND EXISTS "${RENDERDOC_DLL}")
  add_custom_command(TARGET DX12FrameworkSkeleton POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${RENDERDOC_DLL}"
            "$<TARGET_FILE_DIR:DX12FrameworkSkeleton>/renderdoc.dll")
endif()

# ---- Usage hints -----------------------------------------------------------
# Configure:
#   cmake -S . -B build -G "Visual Studio 17 2022" -A x64 \
#         -DENABLE_RENDERDOC=ON -DUSE_DXC=OFF \
#         -DRENDERDOC_INCLUDE="C:/Program Files/RenderDoc/include" \
#         -DRENDERDOC_DLL="C:/Program Files/RenderDoc/renderdoc.dll"
#
# Build:
#   cmake --build build --config Debug
#
# Run (recommended) via RenderDoc "Launch Application" or run the exe directly.
