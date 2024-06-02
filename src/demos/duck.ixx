module;
#include "utils.h"

export module duck_demo;
export import demo;

export class DuckDemo : public IDemo
{
    static constexpr float cDropTime = 0.05f;

public:
    DuckDemo(IRenderer* pRenderer);

    void init() override;
    void update(float dt) override;
    void draw(GlobalCB& cb) override;
    void processInput(IWindow::Message msg, float dt) override;

private:
};

module :private;

DuckDemo::DuckDemo(IRenderer* pRenderer) : IDemo{"DuckDemo", pRenderer}
{
    mpSurface = std::make_unique<WaterSurface>(mpRenderer, 10.0f, 256);
}

void DuckDemo::init()
{
    IDemo::init();

}

void DuckDemo::draw(GlobalCB& cb)
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it
    ID3D11DeviceContext* const pContext = pDX11Renderer->getContext();
}

void DuckDemo::processInput(IWindow::Message msg, float dt)
{

}

void DuckDemo::update(float dt)
{
    IDemo::update(dt);
}