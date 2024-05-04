module;

#include <d3d11.h>

export module dxrenderer;
export import core.renderer;

import std.core;

export class DXRenderer : public IRenderer
{
public:
    DXRenderer(std::weak_ptr<IWindow> pWindow) : IRenderer{pWindow}
    {
    }
};