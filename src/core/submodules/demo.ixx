module;

#include "utils.h"

export module core.demo;
export import core.context;
export import core.camera;
export import core.renderer;
export import surface;
import window;

export class IDemo
{
public:
    virtual ~IDemo();

    virtual void init()
    {
        if (mpSurface != nullptr)
        {
            mpSurface->init();
        }
    }

    virtual void draw()
    {
        if (mpSurface != nullptr)
        {
            mpSurface->draw();
        }
    }

    virtual void processInput(IWindow::Message msg, float dt) = 0;
    virtual void update(float dt)
    {
        if (mpSurface != nullptr)
        {
            mpSurface->update(dt);
        }
    }

    virtual void renderUi()
    {

    }

    std::function<void()> mpMenuBarCallback;

    const char* mpDemoName;
    Camera mCamera{0.0f, 0.5f, 10.0f};

protected:
    IDemo(Context& ctx, const char* demoName, IRenderer* pRenderer, std::shared_ptr<IWindow> pWindow, std::unique_ptr<ISurface> pSurface = nullptr)
        : mCtx{ctx}, mpRenderer{pRenderer}, mpDemoName{demoName}, mpSurface{std::move(pSurface)}, mpWindow{pWindow}
    {

    }

    std::unique_ptr<ISurface> mpSurface;
    IRenderer* mpRenderer;
    Context& mCtx;
    std::shared_ptr<IWindow> mpWindow;
};

module :private;

IDemo::~IDemo()
{
    mCtx.renderableSelection.renderableIds.clear();
    mCtx.lastXMousePosition = -1;
    mCtx.lastYMousePosition = -1;
}
