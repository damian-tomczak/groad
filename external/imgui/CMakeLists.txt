project(ImGui)

add_subdirectory(modules)

set(SRC_FILES
    src/backend/imgui_impl_dx11.cpp
    src/backend/imgui_impl_win32.cpp
    src/imgui_demo.cpp
    src/imgui_draw.cpp
    src/imgui_tables.cpp
    src/imgui_widgets.cpp
    src/imgui.cpp
)

set(H_FILES
    include/imconfig.h
    include/imgui_impl_dx11.h
    include/imgui_impl_win32.h
    include/imgui_internal.h
    include/imgui.h
    include/imstb_textedit.h
    src/imstb_rectpack.h
    src/imstb_truetype.h
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
