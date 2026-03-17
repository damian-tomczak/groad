module;

#include "imgui.h"

#include "utils.h"

export module sso_demo;

import utils;
export import core.demo;
import dx11renderer;
import dx12renderer;

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

    auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);
}

void SSODemo::draw()
{
    IDemo::draw();

    auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);
}

void SSODemo::processInput(IWindow::Message msg, float dt)
{
}

void SSODemo::renderUi()
{
    auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);
}

void SSODemo::update(float dt)
{
    IDemo::update(dt);
}

