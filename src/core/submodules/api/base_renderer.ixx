module;

#include "utils.h"

export module core.renderer;

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

export class IRenderable : public NonCopyable
{
public:
    using Id = unsigned;

    IRenderable(std::string_view tag) : mTag{tag}, id{counter++}
    {
    }

    std::string mTag;
    Id id;
    bool isVisible = true;

    virtual const std::vector<float>& getGeometry() const
    {
        return mGeometry;
    }

    virtual const std::vector<unsigned>& getTopology() const
    {
        return mTopology;
    }

    void regenerateData()
    {
        generateGeometry();
        generateTopology();
    }

protected:
    std::vector<float> mGeometry;
    std::vector<unsigned> mTopology;

    virtual void generateGeometry() = 0;
    virtual void generateTopology() = 0;

private:
    inline static Id counter = 0;
};

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