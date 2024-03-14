module;

#include <DirectXMath.h>
#include <Windows.h>

#include <d3d11.h>

#include "utils.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "mg.hpp"

export module app;

import core;
import core.options;
import window;
import dx11renderer;
import torus;

using namespace std::chrono;

export class App final : public NonCopyableAndMoveable
{
    static constexpr float aspectRatioFactor = 0.25f;

public:
    App(const ParsedOptions&& options);
    ~App();

    void init();
    void onResize();
    void run();

    void updateScene(float deltaTime);
    void renderScene();

private:
    void processInput(IWindow::Message msg, float deltaTime);
    void renderUi();

    DirectX::XMFLOAT4X4 mWorldMatrix;
    DirectX::XMFLOAT4X4 mViewMatrix;

    Torus mTorus{0.7f, 0.2f, 100, 20};

    const ParsedOptions mOptions;
    Camera mCamera{0.f, 0.f, -5.f};

    std::shared_ptr<IWindow> mpWindow;
    std::unique_ptr<DX11Renderer> mpRenderer;

    bool mIsRunning{};
    bool mIsMenuEnabled{};
    bool mIsUiClicked{};
    bool mIsLeftMouseClicked{};
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
}

App::~App()
{
    mpWindow.reset();
    mpRenderer.reset();

    ImGui::DestroyContext();
}

void App::init()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImGui::StyleColorsDark();

    mpWindow = std::shared_ptr<IWindow>(IWindow::createWindow());
    mpWindow->init();
    mpWindow->show();

    const auto dx11renderer = dynamic_cast<DX11Renderer*>(IRenderer::createRenderer(mOptions.api, mpWindow));
    ASSERT(dx11renderer != nullptr);
    mpRenderer = std::unique_ptr<DX11Renderer>(dx11renderer);
    mpRenderer->init(mTorus.getGeometry(), mTorus.getTopology());

    onResize();
}

void App::onResize()
{
    ASSERT(mpWindow);
    mpRenderer->onResize();
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
        const float dt = static_cast<float>(elapsedNanoseconds) / nanosecondsPerSecond;
        previousTime = nowTime;

        IWindow::Message msg = mpWindow->getMessage();
        while (msg != IWindow::Message::EMPTY)
        {
            processInput(msg, dt);
            msg = mpWindow->getMessage();
        }

        updateScene(dt);
        renderScene();
    }
}

void App::updateScene(float dt)
{
}

void App::renderScene()
{
    mpRenderer->getImmediateContext()->ClearRenderTargetView(mpRenderer->getRenderTargetView(),
                                                             reinterpret_cast<const float*>(&Colors::Blue));
    mpRenderer->getImmediateContext()->ClearDepthStencilView(mpRenderer->getDepthStencilView(),
                                                             D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    mpRenderer->getImmediateContext()->IASetInputLayout(mpRenderer->getInputLayout());
    mpRenderer->getImmediateContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    UINT vStride = sizeof(DirectX::XMFLOAT3), offset = 0;
    mpRenderer->getImmediateContext()->IASetVertexBuffers(0, 1, mpRenderer->getVertexBuffer(), &vStride, &offset);
    mpRenderer->getImmediateContext()->IASetIndexBuffer(mpRenderer->getIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
    mpRenderer->getImmediateContext()->RSSetState(mpRenderer->getWireframeRS());

    DirectX::XMMATRIX world = XMLoadFloat4x4(&mWorldMatrix);
    DirectX::XMMATRIX view = mCamera.getViewMatrix();
    DirectX::XMMATRIX projectionMatrix = mg::createPerspectiveFovLH(DirectX::XMConvertToRadians(mCamera.getZoom()),
                                                                    mpWindow->getAspectRatio(), 1.0f, 1000.0f);
    DirectX::XMMATRIX worldViewProj = XMMatrixTranspose(world * view * projectionMatrix);

    D3D11_MAPPED_SUBRESOURCE cbData;
    mpRenderer->getImmediateContext()->Map(mpRenderer->getConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
    memcpy(cbData.pData, &worldViewProj, sizeof(worldViewProj));
    mpRenderer->getImmediateContext()->Unmap(mpRenderer->getConstantBuffer(), 0);

    mpRenderer->getImmediateContext()->VSSetShader(mpRenderer->getVertexShader(), nullptr, 0);
    mpRenderer->getImmediateContext()->PSSetShader(mpRenderer->getFragmentShader(), nullptr, 0);
    mpRenderer->getImmediateContext()->VSSetConstantBuffers(0, 1, mpRenderer->getAddressOfConstantBuffer());
    mpRenderer->getImmediateContext()->DrawIndexed(static_cast<UINT>(mTorus.getTopology().size()), 0, 0);

    renderUi();

    mpRenderer->getSwapchain()->Present(0, 0);
}

void App::processInput(IWindow::Message msg, float deltaTime)
{
    switch (msg)
    {
    case IWindow::Message::QUIT:
        mIsRunning = false;
        break;
    case IWindow::Message::RESIZE:
        onResize();
        break;
    case IWindow::Message::KEY_W_DOWN:
        mCamera.moveCamera(Camera::FORWARD, deltaTime);
        break;
    case IWindow::Message::KEY_S_DOWN:
        mCamera.moveCamera(Camera::BACKWARD, deltaTime);
        break;
    case IWindow::Message::KEY_A_DOWN:
        mCamera.moveCamera(Camera::LEFT, deltaTime);
        break;
    case IWindow::Message::KEY_D_DOWN:
        mCamera.moveCamera(Camera::RIGHT, deltaTime);
        break;
    case IWindow::Message::MOUSE_LEFT_DOWN:
        mIsLeftMouseClicked = true;
        break;
    case IWindow::Message::MOUSE_LEFT_UP:
        mIsLeftMouseClicked = false;
        break;
    case IWindow::Message::MOUSE_MOVE:
    {
        if (mIsLeftMouseClicked && (!mIsUiClicked))
        {
            const auto mousePosition = mpWindow->getEvent<IWindow::Event::MousePosition>();
            mCamera.rotateCamera(static_cast<float>(mousePosition.xoffset), static_cast<float>(-mousePosition.yoffset));
        }
        break;
    }
    case IWindow::Message::MOUSE_WHEEL:
        const auto mouseWheel = mpWindow->getEvent<IWindow::Event::MouseWheel>();
        mCamera.setZoom(static_cast<float>(mouseWheel.yoffset));
        break;
    };
}

void App::renderUi()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    mIsMenuEnabled = false;

    const ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    const float menuWidth = mpWindow->getWidth() * 0.2f;

    ImGui::SetNextWindowPos(ImVec2{mpWindow->getWidth() - menuWidth, 0});
    ImGui::SetNextWindowSize(ImVec2{menuWidth, static_cast<float>(mpWindow->getHeight())});

    if (ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        mIsMenuEnabled = true;
    }

    ImGui::Text("Torus:");

    mIsUiClicked = false;

    ImGui::Spacing();

    bool isGeometryChanged{};
    bool isLastItemActive{};

    ImGui::Text("Major Radius:");
    ImGui::SliderFloat("##majorRadius", &mTorus.mMajorRadius, 0.0f, 1.0f);
    isLastItemActive = ImGui::IsItemActive();
    mIsUiClicked |= isLastItemActive;
    isGeometryChanged |= isLastItemActive;

    ImGui::Text("Minor Radius:");
    ImGui::SliderFloat("##minorRadius", &mTorus.mMinorRadius, 0.0f, 1.0f);
    isLastItemActive = ImGui::IsItemActive();
    mIsUiClicked |= isLastItemActive;
    isGeometryChanged |= isLastItemActive;

    ImGui::Spacing();

    bool isTopologyChanged{};

    ImGui::Text("Major Segments:");
    ImGui::SliderInt("##majorSegments", &mTorus.mMajorSegments, 3, 100);
    isLastItemActive = ImGui::IsItemActive();
    mIsUiClicked |= isLastItemActive;
    isTopologyChanged |= isLastItemActive;
    ImGui::Text("Minor Segments:");
    ImGui::SliderInt("##minorSegments", &mTorus.mMinorSegments, 3, 100);
    isLastItemActive = ImGui::IsItemActive();
    mIsUiClicked |= isLastItemActive;
    isTopologyChanged |= isLastItemActive;

    if (isGeometryChanged || isTopologyChanged)
    {
        mpRenderer->buildGeometryBuffers(mTorus.getGeometry(), mTorus.getTopology());

        mTorus.generateGeometry();
        mTorus.generateTopology();
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}