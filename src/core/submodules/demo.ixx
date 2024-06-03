module;

#include "utils.h"

export module demo;
export import dx11renderer;
export import surface;
export import core;
export import dx11renderer;
export import window;

export class IDemo
{
public:
    virtual void init()
    {
        mpSurface->init();
    }
    virtual void draw(GlobalCB& cb) = 0;
    virtual void processInput(IWindow::Message msg, float dt) = 0;
    virtual void update(float dt)
    {
        mpSurface->update(dt);
    }

    const char* mpDemoName;

protected:
    IDemo(const char* demoName, IRenderer* pRenderer) : mpRenderer{pRenderer}, mpDemoName{demoName}
    {

    }

    std::unique_ptr<ISurface> mpSurface;
    IRenderer* mpRenderer;
};