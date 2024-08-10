option(ASSIMP_BUILD_TESTS OFF)
option(ASSIMP_INSTALL OFF)
add_subdirectory(ImGui)
# TODO: remove DXTK
add_subdirectory(DirectXTK)
# University requirement
add_subdirectory(Scene-Serializer/cpp/Serializer)