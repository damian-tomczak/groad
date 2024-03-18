module;

#include "utils.h"

export module renderer;

import window;
import std.core;

export enum class API : uint8_t
{
    DEFAULT,
    DX11,
    DX12,
    OGL,
    VULKAN,
    COUNT
};

export constexpr const char* apiToStr(const API api)
{
    switch (api)
    {
    case API::DX11:
        return "Directx 11";
    case API::DX12:
        return "Directx 12";
    case API::OGL:
        return "OpenGL";
    case API::VULKAN:
        return "Vulkan";
    default:
        return "INVALID_GRAPHICS_API";
    }
}

export class IRenderer : public NonCopyableAndMoveable
{
    inline static bool isRendererCreated{};

public:
    IRenderer(std::weak_ptr<IWindow> pWindow) : mpWindow{pWindow}
    {
    }
    ~IRenderer()
    {
        isRendererCreated = false;
    }

    virtual void init() = 0;
    virtual void onResize() = 0;

    static IRenderer* const createRenderer(const API selectedApi, std::weak_ptr<IWindow> pWindow);

protected:
    std::weak_ptr<IWindow> mpWindow;
};