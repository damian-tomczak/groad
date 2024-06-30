option(ASSIMP_BUILD_TESTS OFF)
option(ASSIMP_INSTALL OFF)
add_subdirectory(imgui)
# I don't like the idea adding the whole DirectXTK to the project, but I don't see any other better substitution.
add_subdirectory(DirectXTK)