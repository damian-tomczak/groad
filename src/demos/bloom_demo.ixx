module;
#include "utils.h"
#include "imgui.h"

#include <d3d11.h>
#include <cmath>
#include <algorithm>
#include <array>
#include <memory>
#include <numbers>
#include <vector>

export module bloom_demo;

export import core.demo;
import core.renderables;
import dxrenderer;
import window;
import std;

using namespace Microsoft::WRL;
using namespace DirectX;

export class BloomDemo : public IDemo
{
public:
    BloomDemo(Context& ctx, DXRenderer* pRenderer, std::shared_ptr<IWindow> pWindow);

    void init() override;
    void update(float dt) override;
    void draw() override;
    void processInput(IWindow::Message msg, float dt) override;
    void renderUi() override;

private:
    struct RenderTarget
    {
        void create(DXRenderer* pRenderer, UINT width, UINT height, bool withDepth);
        void reset();
        bool isValid() const { return renderTargetView != nullptr; }

        ComPtr<ID3D11Texture2D> texture;
        ComPtr<ID3D11RenderTargetView> renderTargetView;
        ComPtr<ID3D11ShaderResourceView> shaderResourceView;
        ComPtr<ID3D11DepthStencilView> depthStencilView;
        UINT width = 0;
        UINT height = 0;
        bool hasDepth = false;
    };

    struct BloomCB : public CB<ID3D11Buffer>
    {
        XMFLOAT4 params0{}; // x = threshold, y = intensity, z = blur strength
        XMFLOAT4 params1{}; // x = texel size x, y = texel size y, z = direction x, w = direction y
    } mBloomCB{};

    struct GlowObject
    {
        Id id{};
        XMVECTORF32 baseColor{};
        float orbitRadius{};
        float height{};
        float phase{};
    };

    void ensureRenderTargets(UINT width, UINT height);
    void drawGlowObjects();
    void drawScenePass(const D3D11_VIEWPORT& viewport);
    void runBrightPass(const D3D11_VIEWPORT& viewport);
    void runBlurPass(const D3D11_VIEWPORT& viewport, bool horizontal, RenderTarget& target, const RenderTarget& source);
    void runCompositePass();

    std::vector<GlowObject> mGlowObjects;
    RenderTarget mSceneTarget;
    RenderTarget mBrightTarget;
    RenderTarget mBlurTempTarget;
    RenderTarget mBlurTarget;

    ComPtr<ID3D11SamplerState> mpLinearSampler;

    float mBloomThreshold = 1.0f;
    float mBloomIntensity = 1.15f;
    float mBlurStrength = 1.0f;
    float mTime{};
};

module :private;

BloomDemo::BloomDemo(Context& ctx, DXRenderer* pRenderer, std::shared_ptr<IWindow> pWindow)
    : IDemo{ctx, "BloomDemo", pRenderer, pWindow, nullptr}
{
}

void BloomDemo::RenderTarget::reset()
{
    texture.Reset();
    renderTargetView.Reset();
    shaderResourceView.Reset();
    depthStencilView.Reset();
    width = 0;
    height = 0;
    hasDepth = false;
}

void BloomDemo::RenderTarget::create(DXRenderer* pRenderer, UINT targetWidth, UINT targetHeight, bool withDepth)
{
    if ((targetWidth == 0u) || (targetHeight == 0u))
    {
        reset();
        return;
    }

    if ((targetWidth == width) && (targetHeight == height) && (withDepth == hasDepth) && renderTargetView)
    {
        return;
    }

    reset();

    D3D11_TEXTURE2D_DESC textureDesc{
        .Width = targetWidth,
        .Height = targetHeight,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_R16G16B16A16_FLOAT,
        .SampleDesc = {.Count = 1u, .Quality = 0u},
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    };

    HR(pRenderer->getDevice()->CreateTexture2D(&textureDesc, nullptr, texture.GetAddressOf()));
    HR(pRenderer->getDevice()->CreateRenderTargetView(texture.Get(), nullptr, renderTargetView.GetAddressOf()));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    HR(pRenderer->getDevice()->CreateShaderResourceView(texture.Get(), &srvDesc, shaderResourceView.GetAddressOf()));

    if (withDepth)
    {
        D3D11_TEXTURE2D_DESC depthDesc{
            .Width = targetWidth,
            .Height = targetHeight,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_D32_FLOAT,
            .SampleDesc = {.Count = 1u, .Quality = 0u},
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_DEPTH_STENCIL,
        };

        ComPtr<ID3D11Texture2D> depthTexture;
        HR(pRenderer->getDevice()->CreateTexture2D(&depthDesc, nullptr, depthTexture.GetAddressOf()));
        HR(pRenderer->getDevice()->CreateDepthStencilView(depthTexture.Get(), nullptr, depthStencilView.GetAddressOf()));
    }

    width = targetWidth;
    height = targetHeight;
    hasDepth = withDepth;
}

void BloomDemo::init()
{
    IDemo::init();

    mpRenderer->createCB(mBloomCB);
    mpRenderer->getContext()->PSSetConstantBuffers(1, 1, mBloomCB.buffer.GetAddressOf());

    const D3D11_SAMPLER_DESC samplerDesc{
        .Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
        .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
        .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
        .MipLODBias = 0.0f,
        .MaxAnisotropy = 1,
        .ComparisonFunc = D3D11_COMPARISON_NEVER,
        .BorderColor{0.0f, 0.0f, 0.0f, 0.0f},
        .MinLOD = 0.0f,
        .MaxLOD = D3D11_FLOAT32_MAX,
    };

    HR(mpRenderer->getDevice()->CreateSamplerState(&samplerDesc, mpLinearSampler.GetAddressOf()));

    struct GlowInit
    {
        XMVECTORF32 color;
        float radius;
        float height;
        float phase;
    };

    std::array<GlowInit, 4> glowInits{
        GlowInit{XMVECTORF32{4.0f, 1.4f, 0.6f, 1.0f}, 2.6f, 0.0f, 0.0f},
        GlowInit{XMVECTORF32{0.8f, 3.8f, 1.6f, 1.0f}, 3.1f, 0.6f, std::numbers::pi_v<float> / 2.0f},
        GlowInit{XMVECTORF32{0.9f, 1.0f, 4.5f, 1.0f}, 2.2f, -0.4f, std::numbers::pi_v<float>},
        GlowInit{XMVECTORF32{3.5f, 3.2f, 0.9f, 1.0f}, 1.6f, 0.9f, 3.0f * std::numbers::pi_v<float> / 2.0f},
    };

    for (const auto& glowInit : glowInits)
    {
        auto pPoint = std::make_unique<Point>(XMVectorZero(), 0.45f, 36);
        pPoint->setDeletable(false);
        pPoint->mColor = glowInit.color;

        const Id id = pPoint->getId();
        mpRenderer->addRenderable(std::move(pPoint));

        mGlowObjects.push_back({id, glowInit.color, glowInit.radius, glowInit.height, glowInit.phase});
    }

    for (auto& glow : mGlowObjects)
    {
        auto* const pPoint = static_cast<Point*>(mpRenderer->getRenderable(glow.id));
        if (pPoint == nullptr)
        {
            continue;
        }

        const float angle = glow.phase;
        const float x = std::cos(angle) * glow.orbitRadius;
        const float z = std::sin(angle) * glow.orbitRadius + 16.0f;

        pPoint->mWorldPos = XMVectorSet(x, glow.height, z, 1.0f);
    }
}

void BloomDemo::ensureRenderTargets(UINT width, UINT height)
{
    mSceneTarget.create(mpRenderer, width, height, true);
    mBrightTarget.create(mpRenderer, width, height, false);
    mBlurTempTarget.create(mpRenderer, width, height, false);
    mBlurTarget.create(mpRenderer, width, height, false);
}

void BloomDemo::drawGlowObjects()
{
    auto* const pContext = mpRenderer->getContext();

    for (const auto& glow : mGlowObjects)
    {
        auto* const pRenderable = mpRenderer->getRenderable(glow.id);
        if (pRenderable == nullptr)
        {
            continue;
        }

        mpRenderer->mGlobalCB.modelMtx = XMMatrixTranslationFromVector(pRenderable->mWorldPos);
        mpRenderer->mGlobalCB.color = pRenderable->mColor;
        mpRenderer->updateCB(mpRenderer->mGlobalCB);

        const size_t renderableIndex = static_cast<size_t>(glow.id);
        ID3D11Buffer* const* const pVertexBuffer = mpRenderer->getVertexBufferAddress(renderableIndex);
        ID3D11Buffer* const pIndexBuffer = mpRenderer->getIndexBuffer(renderableIndex);

        const UINT stride = pRenderable->getStride();
        const UINT offset = pRenderable->getOffset();
        pContext->IASetVertexBuffers(0, 1, pVertexBuffer, &stride, &offset);
        if (pIndexBuffer != nullptr)
        {
            pContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        }

        pRenderable->draw(mpRenderer, renderableIndex);
    }
}

void BloomDemo::drawScenePass(const D3D11_VIEWPORT& viewport)
{
    auto* const pContext = mpRenderer->getContext();

    pContext->RSSetViewports(1, &viewport);
    ID3D11RenderTargetView* renderTargets[] = {mSceneTarget.renderTargetView.Get()};
    pContext->OMSetRenderTargets(1, renderTargets, mSceneTarget.depthStencilView.Get());

    const float clearColor[]{0.015f, 0.015f, 0.02f, 1.0f};
    pContext->ClearRenderTargetView(mSceneTarget.renderTargetView.Get(), clearColor);
    pContext->ClearDepthStencilView(mSceneTarget.depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    drawGlowObjects();

    ID3D11ShaderResourceView* nullSrvs[]{nullptr};
    pContext->PSSetShaderResources(0, 1, nullSrvs);
}

void BloomDemo::runBrightPass(const D3D11_VIEWPORT& viewport)
{
    auto* const pContext = mpRenderer->getContext();

    pContext->RSSetViewports(1, &viewport);
    ID3D11RenderTargetView* renderTargets[] = {mBrightTarget.renderTargetView.Get()};
    pContext->OMSetRenderTargets(1, renderTargets, nullptr);

    const float clear[]{0.0f, 0.0f, 0.0f, 0.0f};
    pContext->ClearRenderTargetView(mBrightTarget.renderTargetView.Get(), clear);

    const float texelWidth = 1.0f / std::max(viewport.Width, 1.0f);
    const float texelHeight = 1.0f / std::max(viewport.Height, 1.0f);

    mBloomCB.params0 = XMFLOAT4{mBloomThreshold, mBloomIntensity, mBlurStrength, 0.0f};
    mBloomCB.params1 = XMFLOAT4{texelWidth, texelHeight, 0.0f, 0.0f};
    mpRenderer->updateCB(mBloomCB);
    pContext->PSSetConstantBuffers(1, 1, mBloomCB.buffer.GetAddressOf());

    pContext->IASetInputLayout(nullptr);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pContext->VSSetShader(mpRenderer->getShaders().blendVS.first.Get(), nullptr, 0);
    pContext->HSSetShader(nullptr, nullptr, 0);
    pContext->DSSetShader(nullptr, nullptr, 0);
    pContext->GSSetShader(nullptr, nullptr, 0);
    pContext->PSSetShader(mpRenderer->getShaders().bloomExtractPS.first.Get(), nullptr, 0);

    pContext->PSSetSamplers(0, 1, mpLinearSampler.GetAddressOf());
    ID3D11ShaderResourceView* srvs[]{mSceneTarget.shaderResourceView.Get()};
    pContext->PSSetShaderResources(0, 1, srvs);

    pContext->Draw(6, 0);

    ID3D11ShaderResourceView* nullSrvs[]{nullptr};
    pContext->PSSetShaderResources(0, 1, nullSrvs);
}

void BloomDemo::runBlurPass(const D3D11_VIEWPORT& viewport, bool horizontal, RenderTarget& target, const RenderTarget& source)
{
    auto* const pContext = mpRenderer->getContext();

    pContext->RSSetViewports(1, &viewport);
    ID3D11RenderTargetView* renderTargets[] = {target.renderTargetView.Get()};
    pContext->OMSetRenderTargets(1, renderTargets, nullptr);

    const float clear[]{0.0f, 0.0f, 0.0f, 0.0f};
    pContext->ClearRenderTargetView(target.renderTargetView.Get(), clear);

    const float texelWidth = 1.0f / std::max(viewport.Width, 1.0f);
    const float texelHeight = 1.0f / std::max(viewport.Height, 1.0f);
    const float dirX = horizontal ? 1.0f : 0.0f;
    const float dirY = horizontal ? 0.0f : 1.0f;

    mBloomCB.params0 = XMFLOAT4{mBloomThreshold, mBloomIntensity, mBlurStrength, 0.0f};
    mBloomCB.params1 = XMFLOAT4{texelWidth, texelHeight, dirX, dirY};
    mpRenderer->updateCB(mBloomCB);
    pContext->PSSetConstantBuffers(1, 1, mBloomCB.buffer.GetAddressOf());

    pContext->IASetInputLayout(nullptr);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pContext->VSSetShader(mpRenderer->getShaders().blendVS.first.Get(), nullptr, 0);
    pContext->HSSetShader(nullptr, nullptr, 0);
    pContext->DSSetShader(nullptr, nullptr, 0);
    pContext->GSSetShader(nullptr, nullptr, 0);
    pContext->PSSetShader(mpRenderer->getShaders().bloomBlurPS.first.Get(), nullptr, 0);

    pContext->PSSetSamplers(0, 1, mpLinearSampler.GetAddressOf());
    ID3D11ShaderResourceView* srvs[]{source.shaderResourceView.Get()};
    pContext->PSSetShaderResources(0, 1, srvs);

    pContext->Draw(6, 0);

    ID3D11ShaderResourceView* nullSrvs[]{nullptr};
    pContext->PSSetShaderResources(0, 1, nullSrvs);
}

void BloomDemo::runCompositePass()
{
    auto* const pContext = mpRenderer->getContext();

    pContext->RSSetViewports(1, mpRenderer->getAddressOfScreenViewport());
    pContext->OMSetRenderTargets(1, mpRenderer->getAddressOfRenderTargetView(), mpRenderer->getDepthStencilView());

    mBloomCB.params0 = XMFLOAT4{mBloomThreshold, mBloomIntensity, mBlurStrength, 0.0f};
    mBloomCB.params1 = XMFLOAT4{0.0f, 0.0f, 0.0f, 0.0f};
    mpRenderer->updateCB(mBloomCB);
    pContext->PSSetConstantBuffers(1, 1, mBloomCB.buffer.GetAddressOf());

    pContext->IASetInputLayout(nullptr);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pContext->VSSetShader(mpRenderer->getShaders().blendVS.first.Get(), nullptr, 0);
    pContext->HSSetShader(nullptr, nullptr, 0);
    pContext->DSSetShader(nullptr, nullptr, 0);
    pContext->GSSetShader(nullptr, nullptr, 0);
    pContext->PSSetShader(mpRenderer->getShaders().bloomCompositePS.first.Get(), nullptr, 0);

    pContext->PSSetSamplers(0, 1, mpLinearSampler.GetAddressOf());
    ID3D11ShaderResourceView* srvs[]{mSceneTarget.shaderResourceView.Get(), mBlurTarget.shaderResourceView.Get()};
    pContext->PSSetShaderResources(0, 2, srvs);

    pContext->Draw(6, 0);

    ID3D11ShaderResourceView* nullSrvs[]{nullptr, nullptr};
    pContext->PSSetShaderResources(0, 2, nullSrvs);
}

void BloomDemo::draw()
{
    const UINT width = mpWindow->getWidth();
    const UINT height = mpWindow->getHeight();
    if ((width == 0u) || (height == 0u))
    {
        return;
    }

    ensureRenderTargets(width, height);

    D3D11_VIEWPORT viewport{
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = static_cast<float>(width),
        .Height = static_cast<float>(height),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    };

    drawScenePass(viewport);
    runBrightPass(viewport);
    runBlurPass(viewport, true, mBlurTempTarget, mBrightTarget);
    runBlurPass(viewport, false, mBlurTarget, mBlurTempTarget);
    runCompositePass();
}

void BloomDemo::processInput(IWindow::Message msg, float /*dt*/)
{
    switch (msg)
    {
    case IWindow::Message::KEY_R_DOWN:
        mBloomThreshold = 1.0f;
        mBloomIntensity = 1.15f;
        mBlurStrength = 1.0f;
        break;
    default:
        break;
    }
}

void BloomDemo::renderUi()
{
    if (ImGui::Begin("Bloom Controls"))
    {
        ImGui::Text("Use WASD and the mouse to explore the scene.");
        ImGui::SliderFloat("Threshold", &mBloomThreshold, 0.0f, 5.0f);
        ImGui::SliderFloat("Intensity", &mBloomIntensity, 0.0f, 3.0f);
        ImGui::SliderFloat("Blur Strength", &mBlurStrength, 0.2f, 3.0f);
        ImGui::Text("Press R to reset bloom settings.");
    }
    ImGui::End();
}

void BloomDemo::update(float dt)
{
    IDemo::update(dt);

    mTime += dt;

    const float centerZ = 16.0f;
    const float rotationSpeed = 0.35f;

    for (auto& glow : mGlowObjects)
    {
        auto* const pPoint = static_cast<Point*>(mpRenderer->getRenderable(glow.id));
        if (pPoint == nullptr)
        {
            continue;
        }

        const float angle = glow.phase + mTime * rotationSpeed;
        const float x = std::cos(angle) * glow.orbitRadius;
        const float z = std::sin(angle) * glow.orbitRadius + centerZ;

        pPoint->mWorldPos = XMVectorSet(x, glow.height, z, 1.0f);

        const float pulse = 0.75f + 0.25f * std::sin(mTime * 1.8f + glow.phase * 1.3f);
        const XMVECTOR scaledColor = XMVectorScale(glow.baseColor, pulse * 2.0f);
        pPoint->mColor = XMVECTORF32{
            XMVectorGetX(scaledColor),
            XMVectorGetY(scaledColor),
            XMVectorGetZ(scaledColor),
            1.0f};
    }
}
