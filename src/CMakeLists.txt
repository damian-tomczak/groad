if (NOT VK_USE_PLATFORM)
    if (WIN32)
        set(VK_USE_PLATFORM "WIN32")
    endif()
endif()

file(GLOB_RECURSE SRC_FILES
    *.cpp
)

file(GLOB_RECURSE H_FILES
    *.h
)

file(GLOB_RECURSE HPP_FILES
    *.hpp
)

file(GLOB_RECURSE MODULE_FILES
    *.ixx
)

file(GLOB_RECURSE SHADER_FILES
    ${ASSETS_DIR}/shaders/*
)

set(OTHER_FILES
    api/vk/vulkan_functions.inl
)

add_executable(${PROJECT_NAME}
    ${SRC_FILES}
    ${H_FILES}
    ${HPP_FILES}
    ${MODULE_FILES}
    ${SHADER_FILES}
    ${OTHER_FILES}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_sources(${PROJECT_NAME} PRIVATE
    ${EXTERNAL_DIR}/glslang/glslang/ResourceLimits/ResourceLimits.cpp
    ${EXTERNAL_DIR}/glslang/glslang/ResourceLimits/resource_limits_c.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Vulkan-Headers
    glslang
    tinyobjloader
)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ${EXTERNAL_DIR}/glslang # glslang doesn't support modern cmake approach
    ${ASSETS_DIR}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_NAME="${PROJECT_NAME}App"
    ENGINE_NAME="${PROJECT_NAME}Engine"
    _CRT_SECURE_NO_WARNINGS
)

source_group(TREE ${CMAKE_SOURCE_DIR} FILES
    ${SRC_FILES}
    ${H_FILES}
    ${HPP_FILES}
    ${MODULE_FILES}
    ${OTHER_FILES}
)

source_group(TREE ${ASSETS_DIR} FILES
    ${SHADER_FILES}
)

set(OUTPUT_DIR_DEBUG ${CMAKE_BINARY_DIR}/output/${PROJECT_NAME}/Debug)
set(OUTPUT_DIR_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/output/${PROJECT_NAME}/RelWithDebInfo)
set(OUTPUT_DIR_RELEASE ${CMAKE_BINARY_DIR}/output/${PROJECT_NAME}/Release)

file(MAKE_DIRECTORY ${OUTPUT_DIR_DEBUG})
file(MAKE_DIRECTORY ${OUTPUT_DIR_RELWITHDEBINFO})
file(MAKE_DIRECTORY ${OUTPUT_DIR_RELEASE})

set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_DIR_DEBUG}
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_DIR_RELWITHDEBINFO}
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${OUTPUT_DIR_RELWITHDEBINFO}
    VS_DEBUGGER_WORKING_DIRECTORY $<$<CONFIG:Debug>:${OUTPUT_DIR_DEBUG}> $<$<CONFIG:RelWithDebInfo>:${OUTPUT_DIR_RELWITHDEBINFO}> $<$<CONFIG:Release>:${OUTPUT_DIR_RELEASE}>
)

target_compile_options(${PROJECT_NAME} PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Debug>: /Zi /Od /MDd>
        $<$<CONFIG:RelWithDebInfo>: /Zi /O2 /MD>
        $<$<CONFIG:Release>: /O2 /MD>
        /W3 /WX /wd4710
        /experimental:module /wd5050 /scanDependencies # TODO: WO for bleeding modules
    >

    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>: "not implemented" >
)