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
    void drawScene();

private:
    void processInput(float deltaTime);
    void renderUi();

    DirectX::XMFLOAT4X4 mWorldMatrix;
    DirectX::XMFLOAT4X4 mViewMatrix;
    DirectX::XMFLOAT4X4 mProjMatrix;

    const ParsedOptions mOptions;
    Camera mCamera{0.f, 0.f, -5.f};

    std::shared_ptr<IWindow> mpWindow;
    std::unique_ptr<DX11Renderer> mpRenderer;

    bool mIsRunning{};
    bool mIsMenuEnabled{};
    bool mIsUiClicked{};
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
    mpRenderer->init();

    onResize();
}

void App::onResize()
{
    ASSERT(mpWindow);
    mpRenderer->onResize();

    DirectX::XMMATRIX p = mg::createPerspectiveFovLH(aspectRatioFactor * std::numbers::pi_v<float>,
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
    mpRenderer->getImmediateContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    UINT vStride = sizeof(DirectX::XMFLOAT3), offset = 0;
    mpRenderer->getImmediateContext()->IASetVertexBuffers(0, 1, mpRenderer->getVertexBuffer(), &vStride, &offset);
    mpRenderer->getImmediateContext()->IASetIndexBuffer(mpRenderer->getIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
    mpRenderer->getImmediateContext()->RSSetState(mpRenderer->getWireframeRS());

    DirectX::XMMATRIX world = XMLoadFloat4x4(&mWorldMatrix);
    DirectX::XMMATRIX view = mCamera.getViewMatrix();
    DirectX::XMMATRIX proj = XMLoadFloat4x4(&mProjMatrix);
    DirectX::XMMATRIX worldViewProj = XMMatrixTranspose(world * view * proj);

    D3D11_MAPPED_SUBRESOURCE cbData;
    mpRenderer->getImmediateContext()->Map(mpRenderer->getConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
    memcpy(cbData.pData, &worldViewProj, sizeof(worldViewProj));
    mpRenderer->getImmediateContext()->Unmap(mpRenderer->getConstantBuffer(), 0);

    mpRenderer->getImmediateContext()->VSSetShader(mpRenderer->getVertexShader(), nullptr, 0);
    mpRenderer->getImmediateContext()->PSSetShader(mpRenderer->getFragmentShader(), nullptr, 0);
    mpRenderer->getImmediateContext()->VSSetConstantBuffers(0, 1, mpRenderer->getAddressOfConstantBuffer());
    mpRenderer->getImmediateContext()->DrawIndexed(static_cast<UINT>(mpRenderer->mTorus.getTopology().size()), 0, 0);

    renderUi();

    mpRenderer->getSwapchain()->Present(0, 0);
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

void App::renderUi()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    mIsMenuEnabled = false;

    const ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    const float menuWidth = windowSize.x * 0.2f;

    ImGui::SetNextWindowPos(ImVec2(windowSize.x - menuWidth, 0));
    ImGui::SetNextWindowSize(ImVec2(menuWidth, windowSize.y));

    if (ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        mIsMenuEnabled = true;

        ImGui::Text("Ellipsoid:");

        mIsUiClicked = false;

        ImGui::Spacing();

        //static const char* comboItems[] = {"MOVE", "ROTATE"};

        //ImGui::Combo("##InteractionType", reinterpret_cast<int*>(&mInteractionType), comboItems,
        //             IM_ARRAYSIZE(comboItems));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        //ImGui::SliderFloat("a", &properties.a, 0.0f, 10.0f);
        //mIsUiClicked |= ImGui::IsItemActive();
        //ImGui::SliderFloat("b", &properties.b, 0.0f, 10.0f);
        //mIsUiClicked |= ImGui::IsItemActive();
        //ImGui::SliderFloat("c", &properties.c, 0.1f, 10.0f);
        //mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Mouse sensitivity: ");
        //ImGui::SliderFloat("##mouseSensivity", &mMouseSensitivity, 0.1f, 100.0f);

        //ImGui::Text(("DeltaTime: " + std::to_string(deltaTime)).c_str());

        if (ImGui::Button("Reset ellipsoid"))
        {
            mIsUiClicked = true;
        }
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}