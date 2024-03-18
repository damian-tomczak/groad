#include "utils.h"

import std.core;
import dx11renderer;
import core.renderer;
import window;

IRenderer* const IRenderer::createRenderer(const API selectedApi, std::weak_ptr<IWindow> pWindow)
{
    if (isRendererCreated)
    {
        ERR("Renderer is already created");
    }

    IRenderer* pRenderer;

    switch (selectedApi)
    {
    case API::DX11:
        pRenderer = new DX11Renderer(pWindow);
        break;
    default:
        ERR("Selected graphics api is not implemented yet");
    }

    ASSERT(pRenderer != nullptr);

    isRendererCreated = true;

    return pRenderer;
}
