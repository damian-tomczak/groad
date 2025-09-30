#include "utils.h"

import std;
import dx11renderer;
import dx12renderer;
import dxrenderer;
import window;

DXRenderer* const DXRenderer::createRenderer(const API selectedApi, std::weak_ptr<IWindow> pWindow)
{
    if (isRendererCreated)
    {
        ERR("Renderer is already created");
    }

    DXRenderer* pRenderer = nullptr;

    switch (selectedApi)
    {
    case API::DX11:
        pRenderer = new DX11Renderer(pWindow);
        break;
    case API::DX12:
        pRenderer = new DX12Renderer(pWindow);
        break;
    default:
        ERR("Selected graphics api is not implemented yet");
    }

    ASSERT(pRenderer != nullptr);

    isRendererCreated = true;

    return pRenderer;
}
