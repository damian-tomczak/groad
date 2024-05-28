module;

#include "utils.h"

export module scene;
import dx11renderer;
import surface;
import core;
import dx11renderer;

export
{
class IScene
{
public:

    virtual void init()
    {
        mpSurface->init();
    }
    virtual void draw() = 0;
    virtual void processInput() = 0;
    virtual void update(float dt)
    {
        mpSurface->update(dt);
    }

    void drawSurface() const
    {
        mpSurface->draw();
    }

protected:
    IScene(IRenderer* pRenderer) : mpRenderer{pRenderer}
    {

    }

    std::unique_ptr<ISurface> mpSurface;
    IRenderer* mpRenderer;
};

class CadScene : public IScene
{
public:
    CadScene(IRenderer* pRenderer);

    void init() override;
    void update(float dt) override;
    void draw() override;
    void processInput() override;
};

class DuckScene : public IScene
{
public:
    DuckScene(IRenderer* pRenderer);

    void init() override;
    void update(float dt) override;
    void draw() override;
    void processInput() override;
};
}

module :private;

CadScene::CadScene(IRenderer* pRenderer) : IScene{pRenderer}
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

void CadScene::init()
{
    IScene::init();
}

void CadScene::draw()
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it
    ID3D11DeviceContext* const pContext = pDX11Renderer->getContext();
}

void CadScene::processInput()
{

}

void CadScene::update(float dt)
{
    IScene::update(dt);

}

DuckScene::DuckScene(IRenderer* pRenderer) : IScene{pRenderer}
{
    mpSurface = std::make_unique<WaterSurface>(mpRenderer, 10.0f, 10.0f, 256, 256);
}

void DuckScene::init()
{
    IScene::init();

}

void DuckScene::draw()
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it
    ID3D11DeviceContext* const pContext = pDX11Renderer->getContext();
}

void DuckScene::processInput()
{

}

void DuckScene::update(float dt)
{
    IScene::update(dt);
}