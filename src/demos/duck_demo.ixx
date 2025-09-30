module;
#include "utils.h"

export module duck_demo;
export import core.demo;
import core.model_loader;
import dxrenderer;

export class DuckDemo : public IDemo
{
    static constexpr float cDropTime = 0.05f;

public:
    DuckDemo(Context& ctx, DXRenderer* pRenderer, std::shared_ptr<IWindow> pWindow);

    void init() override;
    void update(float dt) override;
    void draw() override;
    void processInput(IWindow::Message msg, float dt) override;

private:
    void drawDuck();
    void drawBox();

private:
    Mesh<PositionNormalLayout> mBox;
    Mesh<TexturedLayout> mDuck;

    ComPtr<ID3D11ShaderResourceView> mDuckTexture;
    ComPtr<ID3D11ShaderResourceView> mpEnvTexture;
    ComPtr<ID3D11SamplerState> mpSampler;
    ComPtr<ID3D11InputLayout> mpDuckLayout;
    ComPtr<ID3D11InputLayout> mpBoxLayout;
};

module :private;

std::pair<std::vector<PositionNormalLayout>, std::vector<short>> createBox(float width, float height, float depth)
{
    std::vector<short> indices = boxIndices();
    std::reverse(indices.begin(), indices.end());
    return {boxVertices(width, height, depth), indices};
}

DuckDemo::DuckDemo(Context& ctx, DXRenderer* pRenderer, std::shared_ptr<IWindow> pWindow)
    : IDemo{ctx, "DuckDemo", pRenderer, pWindow, std::make_unique<WaterSurface>(pRenderer, 10.0f, 256)},
      mBox(mpRenderer, createBox(9.90f, 10.0f, 9.90f)),
      mDuck{MeshLoader::loadMesh<TexturedLayout>(mpRenderer, "assets/meshes/duck.txt")}
{
}

void DuckDemo::init()
{
    IDemo::init();

    mBox.init();
    mDuck.init();

    mDuckTexture = mpRenderer->createShaderResourceView("assets/textures/ducktex.jpg");
    mpEnvTexture = mpRenderer->createShaderResourceView("assets/textures/cubemap.dds");

    const D3D11_SAMPLER_DESC samplerDesc{
        .Filter = D3D11_FILTER_ANISOTROPIC,
        .AddressU = D3D11_TEXTURE_ADDRESS_BORDER,
        .AddressV = D3D11_TEXTURE_ADDRESS_BORDER,
        .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
        .MipLODBias = 0.0f,
        .MaxAnisotropy = 16,
        .ComparisonFunc = D3D11_COMPARISON_NEVER,
        .BorderColor{1.0f, 1.0f, 1.0f, 1.0f},
        .MinLOD = -D3D11_FLOAT32_MAX,
        .MaxLOD = D3D11_FLOAT32_MAX,
    };

    HR(mpRenderer->getDevice()->CreateSamplerState(&samplerDesc, mpSampler.GetAddressOf()));

    HR(mpRenderer->getDevice()->CreateInputLayout(
        TexturedLayout::layout.data(), static_cast<unsigned>(TexturedLayout::layout.size()),
        mpRenderer->getShaders().duckVS.second->GetBufferPointer(),
        mpRenderer->getShaders().duckVS.second->GetBufferSize(), mpDuckLayout.GetAddressOf()));

    HR(mpRenderer->getDevice()->CreateInputLayout(
        PositionNormalLayout::layout.data(), static_cast<unsigned>(PositionNormalLayout::layout.size()),
        mpRenderer->getShaders().wallsVS.second->GetBufferPointer(),
        mpRenderer->getShaders().wallsVS.second->GetBufferSize(), mpBoxLayout.GetAddressOf()));
}

void DuckDemo::draw()
{
    // TODO: 27.7.2024
    IDemo::draw();

    drawDuck();
    drawBox();
}

void DuckDemo::processInput(IWindow::Message msg, float dt)
{

}

void DuckDemo::update(float dt)
{
    IDemo::update(dt);
}

void DuckDemo::drawDuck()
{
    mpRenderer->getContext()->VSSetShader(mpRenderer->getShaders().duckVS.first.Get(), nullptr, 0);
    mpRenderer->getContext()->PSSetShader(mpRenderer->getShaders().duckPS.first.Get(), nullptr, 0);

    mpRenderer->mGlobalCB.modelMtx =
        XMMatrixIdentity() * XMMatrixScaling(0.01f, 0.01f, 0.01f) * XMMatrixTranslation(0.0f, -3.0f, 0.0f);
    mpRenderer->updateCB(mpRenderer->mGlobalCB);

    mpRenderer->getContext()->PSSetShaderResources(0, 1, mDuckTexture.GetAddressOf());
    mpRenderer->getContext()->PSSetSamplers(0, 1, mpSampler.GetAddressOf());

    mpRenderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mpRenderer->getContext()->IASetInputLayout(mpDuckLayout.Get());

    mDuck.draw();
}

void DuckDemo::drawBox()
{
    mpRenderer->getContext()->VSSetShader(mpRenderer->getShaders().wallsVS.first.Get(), nullptr, 0);
    mpRenderer->getContext()->PSSetShader(mpRenderer->getShaders().wallsPS.first.Get(), nullptr, 0);

    mpRenderer->mGlobalCB.modelMtx = XMMatrixScaling(1.f, 1.f, 1.f);
    mpRenderer->updateCB(mpRenderer->mGlobalCB);

    mpRenderer->getContext()->PSSetShaderResources(0, 1, mpEnvTexture.GetAddressOf());
    mpRenderer->getContext()->IASetInputLayout(mpBoxLayout.Get());

    mBox.draw();
}
