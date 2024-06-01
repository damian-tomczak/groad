module;

#include "utils.h"

export module demo;
import dx11renderer;
import surface;
import core;
import dx11renderer;

export
{
class IDemo
{
public:
    virtual void init()
    {
        mpSurface->init();
    }
    virtual void draw(GlobalCB& cb) = 0;
    virtual void processInput() = 0;
    virtual void update(float dt)
    {
        mpSurface->update(dt);
    }

    void drawSurface(GlobalCB& cb) const
    {
        mpSurface->draw(cb);
    }

    const char* mpDemoName;

protected:
    IDemo(const char* demoName, IRenderer* pRenderer) : mpRenderer{pRenderer}, mpDemoName{demoName}
    {

    }

    std::unique_ptr<ISurface> mpSurface;
    IRenderer* mpRenderer;
};

class CADDemo : public IDemo
{
public:
    CADDemo(IRenderer* pRenderer);

    void init() override;
    void update(float dt) override;
    void draw(GlobalCB& cb) override;
    void processInput() override;
};

class DuckDemo : public IDemo
{
public:
    DuckDemo(IRenderer* pRenderer);

    void init() override;
    void update(float dt) override;
    void draw(GlobalCB& cb) override;
    void processInput() override;
};
}

module :private;

CADDemo::CADDemo(IRenderer* pRenderer) : IDemo{"CADDemo", pRenderer}
{
    mpSurface = std::make_unique<GridSurface>(mpRenderer);

    std::unordered_set<IRenderable::Id> selectedPointsIds;

    XMVECTOR pos{-2.f, 3.f, 0};
    auto pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{-1.f, 3.f, 0};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{0.0f, 3.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{1.0f, 3.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{-2.0f, 1.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{-1.0f, 1.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{0.0f, 1.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{1.0f, 1.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    auto pBezier = std::make_unique<BezierC0>(selectedPointsIds, mpRenderer);
    pBezier->regenerateData();
    mpRenderer->addRenderable(std::move(pBezier));
}

void CADDemo::init()
{
    IDemo::init();
}

void CADDemo::draw(GlobalCB& cb)
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it
    ID3D11DeviceContext* const pContext = pDX11Renderer->getContext();
}

void CADDemo::processInput()
{

}

void CADDemo::update(float dt)
{
    IDemo::update(dt);

}

DuckDemo::DuckDemo(IRenderer* pRenderer) : IDemo{"DuckDemo", pRenderer}
{
    mpSurface = std::make_unique<WaterSurface>(mpRenderer, 10.0f, 10.0f, 256, 256);
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

void DuckDemo::processInput()
{

}

void DuckDemo::update(float dt)
{
    IDemo::update(dt);
}