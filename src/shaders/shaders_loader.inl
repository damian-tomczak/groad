#define DEFAULT_SHADER_VER "_5_0"

#define COMPILE_SHADER(shaderName, shaderModel, shaderVar, deviceMethod)                                               \
    compileShader(shaderName, shaderModel DEFAULT_SHADER_VER, shaderVar.second, [this]() {                             \
        return mpDevice->deviceMethod(shaderVar.second->GetBufferPointer(), shaderVar.second->GetBufferSize(),         \
                                      nullptr, &shaderVar.first);                                                      \
    });

#define COMPILE_VERTEX_SHADER(shaderName, shaderVar) COMPILE_SHADER(shaderName, "vs", shaderVar, CreateVertexShader)

#define COMPILE_HULL_SHADER(shaderName, shaderVar) COMPILE_SHADER(shaderName, "hs", shaderVar, CreateHullShader)

#define COMPILE_DOMAIN_SHADER(shaderName, shaderVar) COMPILE_SHADER(shaderName, "ds", shaderVar, CreateDomainShader)

#define COMPILE_GEOMETRY_SHADER(shaderName, shaderVar) COMPILE_SHADER(shaderName, "gs", shaderVar, CreateGeometryShader)

#define COMPILE_PIXEL_SHADER(shaderName, shaderVar) COMPILE_SHADER(shaderName, "ps", shaderVar, CreatePixelShader)

#pragma region torus
    COMPILE_VERTEX_SHADER("default_renderable/default_renderable_vs.hlsl", mShaders.defaultVS);
    COMPILE_PIXEL_SHADER("default_renderable/default_renderable_ps.hlsl", mShaders.defaultPS);
#pragma endregion

#pragma region grid
    COMPILE_VERTEX_SHADER("surfaces/infinite_grid/infinite_grid_vs.hlsl", mShaders.gridVS);
    COMPILE_PIXEL_SHADER("surfaces/infinite_grid/infinite_grid_ps.hlsl", mShaders.gridPS);
#pragma endregion

#pragma region cursor
    COMPILE_VERTEX_SHADER("cursor/cursor_vs.hlsl", mShaders.cursorVS);
    COMPILE_GEOMETRY_SHADER("cursor/cursor_gs.hlsl", mShaders.cursorGS);
    COMPILE_PIXEL_SHADER("cursor/cursor_ps.hlsl", mShaders.cursorPS);
#pragma endregion


#pragma region curve
    COMPILE_VERTEX_SHADER("bezier/bezier_vs.hlsl", mShaders.bezierVS);
    COMPILE_HULL_SHADER("bezier/bezier_hs.hlsl", mShaders.bezierHS);
    COMPILE_DOMAIN_SHADER("bezier/bezier_ds.hlsl", mShaders.bezierDS);
    COMPILE_GEOMETRY_SHADER("bezier/bezier_border_gs.hlsl", mShaders.bezierBorderGS);
    COMPILE_PIXEL_SHADER("bezier/bezier_ps.hlsl", mShaders.bezierPS);
#pragma endregion curves

#pragma region watersurface
    COMPILE_VERTEX_SHADER("surfaces/water_surface/water_surface_vs.hlsl", mShaders.waterSurfaceVS);
    COMPILE_PIXEL_SHADER("surfaces/water_surface/water_surface_ps.hlsl", mShaders.waterSurfacePS);
#pragma endregion watersurface

#pragma region duck
    COMPILE_VERTEX_SHADER("walls/walls_vs.hlsl", mShaders.wallsVS);
    COMPILE_PIXEL_SHADER("walls/walls_ps.hlsl", mShaders.wallsPS);

    COMPILE_VERTEX_SHADER("duck/duck_vs.hlsl", mShaders.duckVS);
    COMPILE_PIXEL_SHADER("duck/duck_ps.hlsl", mShaders.duckPS);
#pragma endregion duck

#pragma region gable
    COMPILE_VERTEX_SHADER("gable/gable_border_vs.hlsl", mShaders.gableBorderVS);
    COMPILE_PIXEL_SHADER("gable/gable_border_ps.hlsl", mShaders.gableBorderPS);

    COMPILE_VERTEX_SHADER("gable/gable_vs.hlsl", mShaders.gableVS);
    COMPILE_HULL_SHADER("gable/gable_hs.hlsl", mShaders.gableHS);
    COMPILE_DOMAIN_SHADER("gable/gable_ds.hlsl", mShaders.gableDS);
    COMPILE_PIXEL_SHADER("gable/gable_ps.hlsl", mShaders.gablePS);
#pragma endregion gable

#pragma region billboard
    COMPILE_VERTEX_SHADER("billboard/billboard_vs.hlsl", mShaders.billboardVS);
    COMPILE_PIXEL_SHADER("billboard/billboard_ps.hlsl", mShaders.billboardPS);
#pragma endregion watersurface
#pragma endregion