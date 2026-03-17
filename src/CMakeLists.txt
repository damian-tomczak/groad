function(os_files_filter files_list_name os_specific_path)
    set(files_list ${${files_list_name}})
    list(FILTER files_list EXCLUDE REGEX "${os_specific_path}/.*/.*")

    set(OS_FILES)
    if(WIN32)
        file(GLOB_RECURSE OS_FILES CONFIGURE_DEPENDS
            "${os_specific_path}/win32/*"
        )
    else()
        message(FATAL_ERROR "Not implemented")
    endif()

    list(LENGTH OS_FILES OS_FILES_LENGTH)
    if(OS_FILES_LENGTH EQUAL 0)
        message(FATAL_ERROR "Files specific for your system could not be found")
    endif()

    list(APPEND files_list ${OS_FILES})
    list(REMOVE_DUPLICATES files_list)
    set(${files_list_name} ${files_list} PARENT_SCOPE)
endfunction()

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE H_FILES CONFIGURE_DEPENDS *.h)
file(GLOB_RECURSE HPP_FILES CONFIGURE_DEPENDS *.hpp)
file(GLOB_RECURSE MODULE_FILES CONFIGURE_DEPENDS *.ixx)
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS shaders/*)
file(GLOB_RECURSE INL_FILES CONFIGURE_DEPENDS *.inl)

os_files_filter(MODULE_FILES core/submodules/platform)

if(MSVC)
    foreach(MODULE_FILE ${MODULE_FILES})
        set_source_files_properties(${MODULE_FILE} PROPERTIES LANGUAGE CXX)
    endforeach()

    foreach(SHADER_FILE ${SHADER_FILES})
        set_source_files_properties(${SHADER_FILE} PROPERTIES VS_TOOL_OVERRIDE "None")
    endforeach()
endif()

add_executable(${CMAKE_PROJECT_NAME})

option(GROAD_WARNINGS_AS_ERRORS "Treat warnings as errors for the groad target." OFF)

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_23)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES CXX_SCAN_FOR_MODULES ON)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${SRC_FILES}
        ${SHADER_FILES}
        ${INL_FILES}
)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            ${MODULE_FILES}
)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES
            ${H_FILES}
            ${HPP_FILES}
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    d3d11
    d3d12
    dxgi
    D3DCompiler
    ImGui::ImGui
    DirectXTK
    MGSerializer
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    .
    ${ASSETS_DIR}
    core
    shaders
)

set(DEBUG_SHADERS_DIR src/shaders)
set(RELEASE_SHADERS_DIR assets/shaders)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    PROJECT_NAME="${CMAKE_PROJECT_NAME}"
    W_PROJECT_NAME=L"${CMAKE_PROJECT_NAME}"
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Debug>:SHADERS_DIR="${DEBUG_SHADERS_DIR}">
    $<$<NOT:$<CONFIG:Debug>>:SHADERS_DIR="${RELEASE_SHADERS_DIR}">
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES
    ${SRC_FILES}
    ${H_FILES}
    ${HPP_FILES}
    ${MODULE_FILES}
    ${INL_FILES}
    ${SHADER_FILES}
)

set(OUTPUT_DIR_DEBUG ${CMAKE_BINARY_DIR}/output/${CMAKE_PROJECT_NAME}/Debug)
set(OUTPUT_DIR_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/output/${CMAKE_PROJECT_NAME}/RelWithDebInfo)
set(OUTPUT_DIR_RELEASE ${CMAKE_BINARY_DIR}/output/${CMAKE_PROJECT_NAME}/Release)

file(MAKE_DIRECTORY ${OUTPUT_DIR_DEBUG})
file(MAKE_DIRECTORY ${OUTPUT_DIR_RELWITHDEBINFO})
file(MAKE_DIRECTORY ${OUTPUT_DIR_RELEASE})

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR_DEBUG}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR_RELEASE}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_DIR_RELWITHDEBINFO}
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

target_compile_options(${CMAKE_PROJECT_NAME} PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Debug>:/Zi /Od /MDd>
        $<$<CONFIG:RelWithDebInfo>:/Zi /O2 /MD>
        $<$<CONFIG:Release>:/O2 /MD>
        /W3
        /EHsc
        /utf-8
        /permissive-
        /Zc:__cplusplus
        /Zc:preprocessor
        $<$<BOOL:${GROAD_WARNINGS_AS_ERRORS}>:/WX>
    >
)

add_custom_target(copy_assets ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${ASSETS_DIR} $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/assets
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>/${DEBUG_SHADERS_DIR}
  COMMENT "Copying assets to build directory"
)

add_dependencies(${PROJECT_NAME} copy_assets)
