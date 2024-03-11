module;

#include <DirectXMath.h>
#include <Windows.h>

// TODO: I won't bother myself right now too much with abstraction
#include <DirectXPackedVector.h>
#include <d3d11.h>

#include "utils.h"

export module app;

import core;
import core.options;
import window;
import dx11renderer;

using namespace std::chrono;

export class App final : public NonCopyableAndNonMoveable
{
    static constexpr float aspectRatioFactor = 0.25f;

public:
    App(const ParsedOptions&& options);

    void init();
    void onResize();
    void run();

    void updateScene(float deltaTime);
    void drawScene();

private:
    void processInput(float deltaTime);

    DirectX::XMFLOAT4X4 mWorldMatrix;
    DirectX::XMFLOAT4X4 mViewMatrix;
    DirectX::XMFLOAT4X4 mProjMatrix;

    const ParsedOptions mOptions;
    Camera mCamera;

    std::shared_ptr<IWindow> mpWindow;
    std::unique_ptr<DX11Renderer> mpRenderer;

    bool mIsRunning{};
};

module :private;

App::App(const ParsedOptions&& options) : mOptions{std::move(options)}
{
    const char* apiStr = apiToStr(mOptions.api);

    LOG("Selected graphics API: " + apiStr);

    if ((mOptions.api != API::DX11) && (mOptions.api != API::DX12))
    {
        ERR(std::format("Selected graphics api {}() is not supported!", apiStr, static_cast<int>(mOptions.api))
                .c_str());
    }

    DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
    XMStoreFloat4x4(&mWorldMatrix, I);
    XMStoreFloat4x4(&mProjMatrix, I);
}

void App::init()
{
    mpWindow = std::shared_ptr<IWindow>(IWindow::createWindow());
    mpWindow->init();
    mpWindow->show();

    const auto dx11renderer = dynamic_cast<DX11Renderer*>(IRenderer::createRenderer(mOptions.api, mpWindow));
    ASSERT(dx11renderer != nullptr);
    mpRenderer = std::unique_ptr<DX11Renderer>(dx11renderer);
    mpRenderer->init();

    onResize();
}

void App::onResize()
{
    ASSERT(mpWindow);
    mpRenderer->onResize();

    DirectX::XMMATRIX p = DirectX::XMMatrixPerspectiveFovLH(aspectRatioFactor * std::numbers::pi_v<float>,
                                                            mpWindow->getAspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProjMatrix, p);
}

void App::run()
{
    mIsRunning = true;
    time_point previousTime = high_resolution_clock::now();
    time_point startTime = steady_clock::now();
    while (mIsRunning)
    {
        const time_point nowTime = high_resolution_clock::now();
        const long long elapsedNanoseconds = duration_cast<nanoseconds>(nowTime - previousTime).count();
        static constexpr float nanosecondsPerSecond = 1e9f;
        const float deltaTime = static_cast<float>(elapsedNanoseconds) / nanosecondsPerSecond;
        previousTime = nowTime;

        processInput(deltaTime);

        updateScene(deltaTime);
        drawScene();
    }
}

void App::updateScene(const float deltaTime)
{
}

void App::drawScene()
{
    mpRenderer->getImmediateContext()->ClearRenderTargetView(mpRenderer->getRenderTargetView(),
                                                             reinterpret_cast<const float*>(&Colors::Black));
    mpRenderer->getImmediateContext()->ClearDepthStencilView(mpRenderer->getDepthStencilView(),
                                                             D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    mpRenderer->getImmediateContext()->IASetInputLayout(mpRenderer->getInputLayout());
    mpRenderer->getImmediateContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT vStride = sizeof(DirectX::XMFLOAT3);
    UINT cStride = sizeof(DirectX::PackedVector::XMCOLOR);
    UINT offset{};
    mpRenderer->getImmediateContext()->IASetVertexBuffers(0, 1, mpRenderer->getVertexBuffer(), &vStride, &offset);
    mpRenderer->getImmediateContext()->IASetIndexBuffer(mpRenderer->getIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

    mpRenderer->getImmediateContext()->RSSetState(mpRenderer->getWireframeRS());

    const DirectX::XMMATRIX world = XMLoadFloat4x4(&mWorldMatrix);
    const DirectX::XMMATRIX view = mCamera.getViewMatrix();
    const DirectX::XMMATRIX proj = XMLoadFloat4x4(&mProjMatrix);
    const auto tmpWorldViewProj = world * view * proj;
    DirectX::XMMATRIX worldViewProj = XMMatrixTranspose(tmpWorldViewProj);

    D3D11_MAPPED_SUBRESOURCE cbData{};
    mpRenderer->getImmediateContext()->Map(mpRenderer->getConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);

    memcpy(cbData.pData, &worldViewProj, sizeof(worldViewProj));

    mpRenderer->getImmediateContext()->Unmap(mpRenderer->getConstantBuffer(), 0);

    mpRenderer->getImmediateContext()->VSSetShader(mpRenderer->getVertexShader(), nullptr, 0);
    mpRenderer->getImmediateContext()->PSSetShader(mpRenderer->getFragmentShader(), nullptr, 0);

    mpRenderer->getImmediateContext()->VSSetConstantBuffers(0, 1, mpRenderer->getAddressOfConstantBuffer());

    mpRenderer->getImmediateContext()->DrawIndexed(static_cast<UINT>(mpRenderer->mTorus.getTopology().size()), 0, 0);

    HR(mpRenderer->getSwapchain()->Present(0, 0));
}

void App::processInput(const float deltaTime)
{
    const auto message = mpWindow->peekMessage();

    switch (message)
    {
    case IWindow::Message::QUIT:
        mIsRunning = false;
        break;
    case IWindow::Message::RESIZE:
        onResize();
        break;
    case IWindow::Message::KEY_W_DOWN:
        mCamera.processKeyboard(Camera::FORWARD, deltaTime);
        break;
    case IWindow::Message::KEY_S_DOWN:
        mCamera.processKeyboard(Camera::BACKWARD, deltaTime);
        break;
    case IWindow::Message::KEY_A_DOWN:
        mCamera.processKeyboard(Camera::LEFT, deltaTime);
        break;
    case IWindow::Message::KEY_D_DOWN:
        mCamera.processKeyboard(Camera::RIGHT, deltaTime);
        break;
    case IWindow::Message::MOUSE_MOVE:
        const auto mousePosition = mpWindow->getEvent<IWindow::Event::MousePosition>();
        mCamera.processMouseMovement(static_cast<float>(mousePosition.xoffset),
                                     static_cast<float>(mousePosition.yoffset));
        break;
    case IWindow::Message::MOUSE_WHEEL:
        const auto mouseWheel = mpWindow->getEvent<IWindow::Event::MouseWheel>();
        mCamera.processMouseScroll(static_cast<float>(mouseWheel.yoffset));
        break;
    };

    mpWindow->dispatchMessage();
}
