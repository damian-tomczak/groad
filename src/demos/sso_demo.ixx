module;
#include "utils.h"

#include "imgui.h"

export module sso_demo;
export import core.demo;
import dxrenderer;

export class SSODemo : public IDemo
{
public:
    SSODemo(Context& ctx, IRenderer* pRenderer, std::shared_ptr<IWindow> pWindow);

    void init() override;
    void update(float dt) override;
    void draw() override;
    void processInput(IWindow::Message msg, float dt) override;
    void renderUi() override;

private:
};

module :private;

SSODemo::SSODemo(Context& ctx, IRenderer* pRenderer, std::shared_ptr<IWindow> pWindow)
    : IDemo{ctx, "SSODemo", pRenderer, pWindow, nullptr}
{
}

void SSODemo::init()
{
    IDemo::init();

    ASSERT(dynamic_cast<DXRenderer*>(mpRenderer) != nullptr);
}

void SSODemo::draw()
{
    IDemo::draw();

    ASSERT(dynamic_cast<DXRenderer*>(mpRenderer) != nullptr);
}

void SSODemo::processInput(IWindow::Message msg, float dt)
{
}

void SSODemo::renderUi()
{
    ASSERT(dynamic_cast<DXRenderer*>(mpRenderer) != nullptr);
}

void SSODemo::update(float dt)
{
    IDemo::update(dt);
}

