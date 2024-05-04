module;

#include <DirectXMath.h>

#include "utils.h"

export module core.renderer;

import window;
import std.core;

using namespace DirectX;

// clang-format off
export namespace Colors
{
    inline constexpr XMFLOAT4 White  {1.0f, 1.0f, 1.0f, 1.0f};
    inline constexpr XMFLOAT4 Black  {0.0f, 0.0f, 0.0f, 1.0f};
    inline constexpr XMFLOAT4 Red    {1.0f, 0.0f, 0.0f, 1.0f};
    inline constexpr XMFLOAT4 Green  {0.0f, 1.0f, 0.0f, 1.0f};
    inline constexpr XMFLOAT4 Blue   {0.0f, 0.0f, 1.0f, 1.0f};
    inline constexpr XMFLOAT4 Yellow {1.0f, 1.0f, 0.0f, 1.0f};
    inline constexpr XMFLOAT4 Cyan   {0.0f, 1.0f, 1.0f, 1.0f};
    inline constexpr XMFLOAT4 Magenta{1.0f, 0.0f, 1.0f, 1.0f};
}
// clang-format on

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

    IRenderable(FXMVECTOR pos, std::string_view tag) : mTag{tag}, id{counter++}, mWorldPos{pos}
    {
    }

    std::string mTag;
    Id id;
    bool isVisible = true;

    XMVECTOR mLocalPos{};
    XMVECTOR mWorldPos{};

    float mPitch{};
    float mYaw{};
    float mRoll{};
    float mScale = 1.f;

    virtual const std::vector<XMFLOAT3>& getGeometry() const
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

    virtual void generateGeometry()
    {
    }

    virtual void generateTopology()
    {
    }

protected:
    std::vector<XMFLOAT3> mGeometry;
    std::vector<unsigned> mTopology;

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
    virtual class IRenderable* getRenderable(IRenderable::Id id) const = 0;

    std::vector<std::unique_ptr<IRenderable>> mRenderables;

protected:
    std::weak_ptr<IWindow> mpWindow;
};