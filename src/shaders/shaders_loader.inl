#pragma region torus
    compileShader("torus/default_renderable_vs.hlsl", "vs_5_0", mShaders.defaultVS.second, [this]() {
        return mpDevice->CreateVertexShader(mShaders.defaultVS.second->GetBufferPointer(), mShaders.defaultVS.second->GetBufferSize(),
                                                 nullptr, &mShaders.defaultVS.first);
    });

    compileShader("torus/default_renderable_ps.hlsl", "ps_5_0", mShaders.defaultPS.second, [this]() {
        return mpDevice->CreatePixelShader(mShaders.defaultPS.second->GetBufferPointer(), mShaders.defaultPS.second->GetBufferSize(),
                                                nullptr, &mShaders.defaultPS.first);
    });
#pragma endregion

#pragma region grid
    compileShader("surfaces/infinite_grid/infinite_grid_vs.hlsl", "vs_5_0", mShaders.gridVS.second, [this]() {
        return mpDevice->CreateVertexShader(mShaders.gridVS.second->GetBufferPointer(), mShaders.gridVS.second->GetBufferSize(),
                                            nullptr, &mShaders.gridVS.first);
    });
    compileShader("surfaces/infinite_grid/infinite_grid_ps.hlsl", "ps_5_0", mShaders.gridPS.second, [this]() {
        return mpDevice->CreatePixelShader(mShaders.gridPS.second->GetBufferPointer(), mShaders.gridPS.second->GetBufferSize(),
                                           nullptr,
                                           &mShaders.gridPS.first);
    });
#pragma endregion

#pragma region cursor
    compileShader("cursor/cursor_vs.hlsl", "vs_5_0", mShaders.cursorVS.second, [this]() {
        return mpDevice->CreateVertexShader(mShaders.cursorVS.second->GetBufferPointer(), mShaders.cursorVS.second->GetBufferSize(),
                                            nullptr, &mShaders.cursorVS.first);
    });
    compileShader("cursor/cursor_gs.hlsl", "gs_5_0", mShaders.cursorGS.second, [this]() {
        return mpDevice->CreateGeometryShader(mShaders.cursorGS.second->GetBufferPointer(), mShaders.cursorGS.second->GetBufferSize(),
                                              nullptr, &mShaders.cursorGS.first);
    });
    compileShader("cursor/cursor_ps.hlsl", "ps_5_0", mShaders.cursorPS.second, [this]() {
        return mpDevice->CreatePixelShader(mShaders.cursorPS.second->GetBufferPointer(), mShaders.cursorPS.second->GetBufferSize(),
                                           nullptr,
                                           &mShaders.cursorPS.first);
    });
#pragma endregion


#pragma region beziers
#pragma region bezier_c0
    compileShader("beziers/bezier_c0/bezier_c0_vs.hlsl", "vs_5_0", mShaders.bezierC0VS.second, [this]() {
        return mpDevice->CreateVertexShader(mShaders.bezierC0VS.second->GetBufferPointer(), mShaders.bezierC0VS.second->GetBufferSize(),
                                            nullptr, &mShaders.bezierC0VS.first);
    });
    compileShader("beziers/bezier_c0/bezier_c0_hs.hlsl", "hs_5_0", mShaders.bezierC0HS.second, [this]() {
        return mpDevice->CreateHullShader(mShaders.bezierC0HS.second->GetBufferPointer(), mShaders.bezierC0HS.second->GetBufferSize(),
                                          nullptr,
                                          &mShaders.bezierC0HS.first);
    });
    compileShader("beziers/bezier_c0/bezier_c0_ds.hlsl", "ds_5_0", mShaders.bezierC0DS.second, [this]() {
        return mpDevice->CreateDomainShader(mShaders.bezierC0DS.second->GetBufferPointer(), mShaders.bezierC0DS.second->GetBufferSize(),
                                            nullptr, &mShaders.bezierC0DS.first);
    });
    compileShader("beziers/bezier_c0/bezier_c0_border_gs.hlsl", "gs_5_0", mShaders.bezierC0BorderGS.second, [this]() {
        return mpDevice->CreateGeometryShader(mShaders.bezierC0BorderGS.second->GetBufferPointer(), mShaders.bezierC0BorderGS.second->GetBufferSize(),
                                              nullptr, &mShaders.bezierC0BorderGS.first);
    });
    compileShader("beziers/bezier_c0/bezier_c0_ps.hlsl", "ps_5_0", mShaders.bezierC0PS.second, [this]() {
        return mpDevice->CreatePixelShader(mShaders.bezierC0PS.second->GetBufferPointer(), mShaders.bezierC0PS.second->GetBufferSize(),
                                           nullptr,
                                           &mShaders.bezierC0PS.first);
    });
#pragma endregion bezier_c0

#pragma region bezier_c2
    // Include similar lambda function calls as above for the Bezier C2 shaders
#pragma endregion bezier_c2

#pragma region watersurface
    compileShader("surfaces/water_surface/water_surface_vs.hlsl", "vs_5_0", mShaders.waterSurfaceVS.second, [this]() {
        return mpDevice->CreateVertexShader(mShaders.waterSurfaceVS.second->GetBufferPointer(), mShaders.waterSurfaceVS.second->GetBufferSize(), nullptr,
                                           &mShaders.waterSurfaceVS.first);
    });
    compileShader("surfaces/water_surface/water_surface_ps.hlsl", "ps_5_0", mShaders.waterSurfacePS.second, [this]() {
        return mpDevice->CreatePixelShader(mShaders.waterSurfacePS.second->GetBufferPointer(), mShaders.waterSurfacePS.second->GetBufferSize(), nullptr,
                                           &mShaders.waterSurfacePS.first);
    });
#pragma endregion watersurface

#pragma region duck
    compileShader("walls/walls_vs.hlsl", "vs_5_0", mShaders.wallsVS.second, [this]() {
        return mpDevice->CreateVertexShader(mShaders.wallsVS.second->GetBufferPointer(),
                                            mShaders.wallsVS.second->GetBufferSize(), nullptr,
                                            &mShaders.wallsVS.first);
    });
    compileShader("walls/walls_ps.hlsl", "ps_5_0", mShaders.wallsPS.second, [this]() {
        return mpDevice->CreatePixelShader(mShaders.wallsPS.second->GetBufferPointer(),
                                           mShaders.wallsPS.second->GetBufferSize(), nullptr,
                                           &mShaders.wallsPS.first);
    });

    compileShader("duck/duck_vs.hlsl", "vs_5_0", mShaders.duckVS.second, [this]() {
        return mpDevice->CreateVertexShader(mShaders.duckVS.second->GetBufferPointer(),
                                            mShaders.duckVS.second->GetBufferSize(), nullptr, &mShaders.duckVS.first);
    });
    compileShader("duck/duck_ps.hlsl", "ps_5_0", mShaders.duckPS.second, [this]() {
        return mpDevice->CreatePixelShader(mShaders.duckPS.second->GetBufferPointer(),
                                           mShaders.duckPS.second->GetBufferSize(), nullptr, &mShaders.duckPS.first);
    });
#pragma endregion duck

#pragma region gable
    compileShader("gable/gable_border_vs.hlsl", "vs_5_0", mShaders.gableBorderVS.second, [this]() {
        return mpDevice->CreateVertexShader(mShaders.gableBorderVS.second->GetBufferPointer(),
                                            mShaders.gableBorderVS.second->GetBufferSize(), nullptr, &mShaders.gableBorderVS.first);
    });
    compileShader("gable/gable_border_ps.hlsl", "ps_5_0", mShaders.gableBorderPS.second, [this]() {
        return mpDevice->CreatePixelShader(mShaders.gableBorderPS.second->GetBufferPointer(),
                                           mShaders.gableBorderPS.second->GetBufferSize(), nullptr, &mShaders.gableBorderPS.first);
    });

    compileShader("gable/gable_vs.hlsl", "vs_5_0", mShaders.gableVS.second, [this]() {
        return mpDevice->CreateVertexShader(mShaders.gableVS.second->GetBufferPointer(),
                                            mShaders.gableVS.second->GetBufferSize(), nullptr,
                                            &mShaders.gableVS.first);
    });
    compileShader("gable/gable_hs.hlsl", "hs_5_0", mShaders.gableHS.second, [this]() {
        return mpDevice->CreateHullShader(mShaders.gableHS.second->GetBufferPointer(),
                                          mShaders.gableHS.second->GetBufferSize(), nullptr,
                                          &mShaders.gableHS.first);
    });
    compileShader("gable/gable_ds.hlsl", "ds_5_0", mShaders.gableDS.second, [this]() {
        return mpDevice->CreateDomainShader(mShaders.gableDS.second->GetBufferPointer(),
                                            mShaders.gableDS.second->GetBufferSize(), nullptr,
                                            &mShaders.gableDS.first);
    });
    compileShader("gable/gable_ps.hlsl", "ps_5_0", mShaders.gablePS.second, [this]() {
        return mpDevice->CreatePixelShader(mShaders.gablePS.second->GetBufferPointer(),
                                           mShaders.gablePS.second->GetBufferSize(), nullptr,
                                           &mShaders.gablePS.first);
    });
#pragma endregion gable
#pragma endregion