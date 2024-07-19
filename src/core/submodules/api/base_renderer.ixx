module;

#include "utils.h"

export module core.renderer;

import window;
import std.core;

export
{
    using Color = XMVECTORF32;

    const Color defaultColor = Colors::Blue;
    const Color defaultSelectionColor = Colors::Red;

    enum class API : uint8_t
    {
        DEFAULT,
        DX11,
        DX12,
        OGL,
        VULKAN,
        COUNT
    };

    constexpr const char* apiToStr(const API api)
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

    using Id = int;
    inline constexpr Id invalidId = -1;

    struct IIdentifiable : public NonCopyable
    {
        IIdentifiable(Id id) : id{id}
        {

        }

        Id id;
    };

    class IRenderable : public IIdentifiable
    {
        friend class DX11Renderer;

    public:
        IRenderable(std::string_view tag, FXMVECTOR pos = XMVectorZero(), Color color = defaultColor)
            : IIdentifiable{counter++}, mTag{tag}, mWorldPos{pos}, mColor{color}
        {
        }

        std::string mTag;
        bool isVisible = true;

        XMVECTOR mLocalPos{};
        XMVECTOR mWorldPos{};

        float mPitch{};
        float mYaw{};
        float mRoll{};
        float mScale = 1.f;

        Color mColor;

        const FXMVECTOR getGlobalPos()
        {
            return mLocalPos + mWorldPos;
        }

        virtual const std::vector<XMFLOAT3>& getGeometry() const
        {
            return mGeometry;
        }

        virtual const std::vector<unsigned>& getTopology() const
        {
            return mTopology;
        }

        virtual void regenerateData()
        {
            generateGeometry();
            generateTopology();
        }

        virtual void generateGeometry()
        {
            mGeometry.clear();

            mShouldRebuildBuffers = true;
        }

        virtual void generateTopology()
        {
            mTopology.clear();

            mShouldRebuildBuffers = true;
        }

        virtual bool isScalable()
        {
            return false;
        }

    protected:
        std::vector<XMFLOAT3> mGeometry;
        std::vector<unsigned> mTopology;

    private:
        inline static Id counter = 0;
        bool mShouldRebuildBuffers{};
    };

    class IRenderer : public NonCopyableAndMoveable
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
        virtual class IRenderable* getRenderable(Id id) const = 0;

        virtual void addRenderable(std::unique_ptr<IRenderable>&& renderable) = 0;

        std::vector<std::unique_ptr<IRenderable>> mRenderables;

    protected:
        std::weak_ptr<IWindow> mpWindow;
    };
}