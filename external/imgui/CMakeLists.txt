project(imgui)

file(GLOB_RECURSE SRC_FILES
    *.cpp
)

file(GLOB_RECURSE H_FILES
    *.h
)

add_library(${PROJECT_NAME} STATIC
    ${SRC_FILES}
    ${H_FILES}
)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
)