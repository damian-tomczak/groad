module;

#define NOMINMAX
#include <Windows.h>
#include <cmath>

#include <filesystem>

#include <d3d11.h>
#include <wrl/client.h>

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

import duck_demo;
import cad_demo;
import gable_demo;
import sso_demo;

export class App final : public NonCopyableAndMoveable
{
    static constexpr float aspectRatioFactor = 0.25f;
    static constexpr XMFLOAT4 backgroundColor{0.1f, 0.1f, 0.1f, 1.0f};
    static constexpr int renderablesListWidth = 19;

public:
    App(const ParsedOptions&& options);
    ~App();

    void init();
    void run();

    void update(float dt);
    void draw();

    enum class Demo
    {
        DEFAULT,
        CAD = DEFAULT,
        DUCK,
        GABLE,
        SSO,
    };

    struct Settings
    {
        bool isVsync = true;
    } mSettings{};

private:
    void processInput(IWindow::Message msg, float dt);
    void renderUi();
    void loadDemo(App::Demo mode);
    void checkShaders();

    const ParsedOptions mOptions;

    std::shared_ptr<IWindow> mpWindow;
    DX11Renderer* mpRenderer;

    bool mIsRunning{};

    std::optional<int> mLastXMousePosition;
    std::optional<int> mLastYMousePosition;

    bool mIsWPressed{};
    bool mIsAPressed{};
    bool mIsSPressed{};
    bool mIsDPressed{};

    std::unique_ptr<IDemo> mpDemo;
    Demo mMode{};

    void drawCursor(DX11Renderer::GlobalCB& cb);

    Context mCtx{};

    fs::file_time_type mLastShadersCompilationTime;
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
}

App::~App()
{
    mpWindow.reset();
    mpDemo.reset();

    delete mpRenderer;

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

    mpRenderer = dynamic_cast<DX11Renderer*>(IRenderer::createRenderer(mOptions.api, mpWindow));
    ASSERT(mpRenderer != nullptr);
    mpRenderer->init();

    mpDemo = std::make_unique<CADDemo>(mCtx, mpRenderer, mpWindow);
    mpDemo->init();

    //mpRenderer->createCB(mLights);
}

void App::run()
{
    mIsRunning = true;

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER previousTime;
    QueryPerformanceCounter(&previousTime);

#ifndef NDEBUG
    mLastShadersCompilationTime = fs::file_time_type::clock::now();
#endif

    while (mIsRunning)
    {
#ifndef NDEBUG
        checkShaders();
#endif
        LARGE_INTEGER nowTime;
        QueryPerformanceCounter(&nowTime);

        double elapsedTime = static_cast<double>(nowTime.QuadPart - previousTime.QuadPart) / frequency.QuadPart;
        float dt = static_cast<float>(elapsedTime);

        previousTime = nowTime;

        IWindow::Message msg = mpWindow->getMessage();
        while (msg != IWindow::Message::EMPTY)
        {
            processInput(msg, dt);
            msg = mpWindow->getMessage();
        }

        update(dt);
        draw();
    }
}

void App::update(float dt)
{
    if (mIsWPressed)
    {
        mpDemo->mCamera.moveCamera(Camera::FORWARD, dt);
    }
    else if (mIsAPressed)
    {
        mpDemo->mCamera.moveCamera(Camera::LEFT, dt);
    }
    else if (mIsSPressed)
    {
        mpDemo->mCamera.moveCamera(Camera::BACKWARD, dt);
    }
    else if (mIsDPressed)
    {
        mpDemo->mCamera.moveCamera(Camera::RIGHT, dt);
    }

    mpDemo->update(dt);
}

void App::draw()
{
    auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

    mpRenderer->getContext()->VSSetShader(nullptr, nullptr, 0);
    mpRenderer->getContext()->HSSetShader(nullptr, nullptr, 0);
    mpRenderer->getContext()->DSSetShader(nullptr, nullptr, 0);
    mpRenderer->getContext()->GSSetShader(nullptr, nullptr, 0);
    mpRenderer->getContext()->PSSetShader(nullptr, nullptr, 0);

    mpRenderer->buildGeometryBuffers();

    mpRenderer->getContext()->ClearRenderTargetView(mpRenderer->getRenderTargetView(), reinterpret_cast<const float*>(&backgroundColor));
    mpRenderer->getContext()->ClearDepthStencilView(mpRenderer->getDepthStencilView(),
                                                             D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    const D3D11_DEPTH_STENCIL_DESC depthStencilDesc{
        .DepthEnable = TRUE,
        .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
        .DepthFunc = D3D11_COMPARISON_LESS,
        .StencilEnable = FALSE,
    };

    ComPtr<ID3D11DepthStencilState> pDepthStencilState;
    mpRenderer->getDevice()->CreateDepthStencilState(&depthStencilDesc, pDepthStencilState.GetAddressOf());
    mpRenderer->getContext()->OMSetDepthStencilState(pDepthStencilState.Get(), 1);

    XMMATRIX viewMtx = mpDemo->mCamera.getViewMatrix();
    XMMATRIX projMtx = XMMatrixPerspectiveFovLH(XMConvertToRadians(mpDemo->mCamera.getZoom()),
                                                mpWindow->getAspectRatio(), 0.01f, 100.0f);

    DXRenderer::GlobalCB globalCB
    {
        .modelMtx = XMMatrixIdentity(),
        .view = viewMtx,
        .invView = XMMatrixInverse(nullptr, viewMtx),
        .proj = projMtx,
        .invProj = XMMatrixInverse(nullptr, projMtx),
        .texMtx = XMMatrixIdentity(),
        .cameraPos = mpDemo->mCamera.getPos(),
        .screenWidth = mpWindow->getWidth(),
        .screenHeight = mpWindow->getHeight(),
    };
    pDX11Renderer->mGlobalCB = std::move(globalCB);
    mpRenderer->createCB(pDX11Renderer->mGlobalCB);

    mpRenderer->getContext()->VSSetConstantBuffers(0, 1, pDX11Renderer->mGlobalCB.buffer.GetAddressOf());
    mpRenderer->getContext()->HSSetConstantBuffers(0, 1, pDX11Renderer->mGlobalCB.buffer.GetAddressOf());
    mpRenderer->getContext()->DSSetConstantBuffers(0, 1, pDX11Renderer->mGlobalCB.buffer.GetAddressOf());
    mpRenderer->getContext()->GSSetConstantBuffers(0, 1, pDX11Renderer->mGlobalCB.buffer.GetAddressOf());
    mpRenderer->getContext()->PSSetConstantBuffers(0, 1, pDX11Renderer->mGlobalCB.buffer.GetAddressOf());

    mpRenderer->getContext()->IASetInputLayout(mpRenderer->getDefaultInputLayout());

    mpDemo->draw();

    //drawCursor(pDX11Renderer->mGlobalCB);

    renderUi();

    mpRenderer->getSwapchain()->Present(mSettings.isVsync, 0);
}

void App::processInput(const IWindow::Message msg, float dt)
{
    switch (msg)
    {
    case IWindow::Message::QUIT:
        mIsRunning = false;
        break;
    case IWindow::Message::RESIZE:
    {
        const int width = mpWindow->getWidth();
        const int height = mpWindow->getHeight();
        if ((width != 0) && (height != 0))
        {
            mpRenderer->onResize();
        }
    }
        break;
    case IWindow::Message::KEY_F1:
        loadDemo(Demo::CAD);
        break;
    case IWindow::Message::KEY_F2:
        loadDemo(Demo::DUCK);
        break;
    case IWindow::Message::KEY_F3:
        loadDemo(Demo::GABLE);
        break;
    case IWindow::Message::KEY_W_DOWN:
        if (!mCtx.isUiClicked)
        {
            mIsWPressed = true;
        }
        break;
    case IWindow::Message::KEY_W_UP:
        if (!mCtx.isUiClicked)
        {
            mIsWPressed = false;
        }
        break;
    case IWindow::Message::KEY_S_DOWN:
        if (!mCtx.isUiClicked)
        {
            mIsSPressed = true;
        }
        break;
    case IWindow::Message::KEY_S_UP:
        if (!mCtx.isUiClicked)
        {
            mIsSPressed = false;
        }
        break;
    case IWindow::Message::KEY_A_DOWN:
        if (!mCtx.isUiClicked)
        {
            mIsAPressed = true;
        }
        break;
    case IWindow::Message::KEY_A_UP:
        if (!mCtx.isUiClicked)
        {
            mIsAPressed = false;
        }
        break;
    case IWindow::Message::KEY_D_DOWN:
        if (!mCtx.isUiClicked)
        {
            mIsDPressed = true;
        }
        break;
    case IWindow::Message::KEY_D_UP:
        if (!mCtx.isUiClicked)
        {
            mIsDPressed = false;
        }
        break;
    case IWindow::Message::KEY_CTRL_DOWN:
        mCtx.isCtrlClicked = true;
        break;
    case IWindow::Message::KEY_CTRL_UP:
        mCtx.isCtrlClicked = false;
        break;
    case IWindow::Message::MOUSE_LEFT_UP:
        mCtx.isLeftMouseClicked = false;
        break;
    case IWindow::Message::MOUSE_RIGHT_DOWN:
        mCtx.isRightMouseClicked = true;
        break;
    case IWindow::Message::MOUSE_RIGHT_UP:
        mCtx.isRightMouseClicked = false;
        break;
    case IWindow::Message::KEY_SHIFT_DOWN:
        mCtx.isShiftPressed = true;
        break;
    case IWindow::Message::KEY_SHIFT_UP:
        mCtx.isShiftPressed = false;
        break;
    case IWindow::Message::MOUSE_WHEEL:
        if (!mCtx.isMenuHovered)
        {
            const auto mouseWheel = mpWindow->getEventData<IWindow::Event::MouseWheel>();
            mpDemo->mCamera.setZoom(static_cast<float>(mouseWheel.yOffset));
        }
        break;
    case IWindow::Message::MOUSE_LEFT_DOWN: {
        mCtx.isLeftMouseClicked = true;
    }
    break;
    case IWindow::Message::MOUSE_MIDDLE_DOWN:
        const auto eventData = mpWindow->getEventData<IWindow::Event::MousePosition>();
        mCtx.cursorPos = mpDemo->mCamera.getPos() + mpDemo->mCamera.getFront() * 4.0f;

        for (auto& pRenderable : mpRenderer->getRenderables())
        {
            if (pRenderable != nullptr)
            {
                pRenderable->mLocalPos = pRenderable->mWorldPos - mCtx.cursorPos;
            }
        }
    break;
    case IWindow::Message::MOUSE_MOVE: {
        const auto eventData = mpWindow->getEventData<IWindow::Event::MousePosition>();

        if ((mLastXMousePosition == std::nullopt) && (mLastYMousePosition == std::nullopt))
        {
            mLastXMousePosition = std::make_optional(eventData.x);
            mLastYMousePosition = std::make_optional(eventData.y);
        }

        const float xOffset = (eventData.x - *mLastXMousePosition) * 10 * mouseSensitivity;
        const float yOffset = (eventData.y - *mLastYMousePosition) * 10 * mouseSensitivity;

        mLastXMousePosition = eventData.x;
        mLastYMousePosition = eventData.y;

        if (mCtx.isRightMouseClicked && (!mCtx.isUiClicked))
        {
            mpDemo->mCamera.rotateCamera(xOffset, -yOffset);
        }
    }
    break;
    };

    mpDemo->processInput(msg, dt);

    mpWindow->popEventData();
}

void App::loadDemo(App::Demo mode)
{
    mMode = mode;

    mpRenderer->clearRenderables();

    switch (mMode)
    {
    case App::Demo::CAD:
        mpDemo = std::make_unique<CADDemo>(mCtx, mpRenderer, mpWindow);
        break;
    case App::Demo::DUCK:
        mpDemo = std::make_unique<DuckDemo>(mCtx, mpRenderer, mpWindow);
        break;
    case App::Demo::GABLE:
        mpDemo = std::make_unique<GableDemo>(mCtx, mpRenderer, mpWindow);
        break;
    case App::Demo::SSO:
        mpDemo = std::make_unique<SSODemo>(mCtx, mpRenderer, mpWindow);
        break;
    default:
        break;
    }

    mpDemo->init();
}

void App::checkShaders()
{
    bool shouldRecompileShaders = false;
    for (const auto& entry : fs::recursive_directory_iterator(SHADERS_PATH))
    {
        if (fs::is_regular_file(entry.status()))
        {
            const auto lastModified = fs::last_write_time(entry);
            if (lastModified > mLastShadersCompilationTime)
            {
                shouldRecompileShaders = true;
                mLastShadersCompilationTime = fs::file_time_type::clock::now();
            }
        }
    }

    if (shouldRecompileShaders)
    {
        WARN("Starting recompiling shaders!");
        mpRenderer->createShaders();
        WARN("Shaders recompiled!");
    }
}

void App::drawCursor(DXRenderer::GlobalCB& globalCB)
{
    globalCB.modelMtx = XMMatrixTranslationFromVector(mCtx.cursorPos);

    mpRenderer->updateCB(globalCB);

    mpRenderer->getContext()->VSSetShader(mpRenderer->getShaders().cursorVS.first.Get(), nullptr, 0);
    mpRenderer->getContext()->HSSetShader(nullptr, nullptr, 0);
    mpRenderer->getContext()->DSSetShader(nullptr, nullptr, 0);
    mpRenderer->getContext()->GSSetShader(mpRenderer->getShaders().cursorGS.first.Get(), nullptr, 0);
    mpRenderer->getContext()->PSSetShader(mpRenderer->getShaders().cursorPS.first.Get(), nullptr, 0);
    mpRenderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    mpRenderer->getContext()->Draw(6, 0);
}

std::optional<App::Demo> showMenu(App::Settings& settings, std::function<void()> pMenuBarCallback, const char* demoName, float& menuBarHeight)
{
    std::optional<App::Demo> result{};

    if (ImGui::BeginMainMenuBar())
    {
        menuBarHeight = ImGui::GetFrameHeight();

        if (ImGui::BeginMenu("Demo"))
        {
            if (ImGui::MenuItem("CADDemo", "F1"))
            {
                result = App::Demo::CAD;
            }
            else if (ImGui::MenuItem("DuckDemo", "F2"))
            {
                result = App::Demo::DUCK;
            }
            else if (ImGui::MenuItem("GableDemo", "F3"))
            {
                result = App::Demo::GABLE;
            }
            else if (ImGui::MenuItem("SSODemo", ""))
            {
                result = App::Demo::SSO;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings"))
        {
            ImGui::Checkbox("VSync", &settings.isVsync);

            ImGui::EndMenu();
        }

        ImGui::Text("| Current demo: %s |", demoName);

        if (pMenuBarCallback != nullptr)
        {
            pMenuBarCallback();
        }

        ImGui::EndMainMenuBar();
    }

    return result;
}

void App::renderUi()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    std::optional<App::Demo> selectedMode = showMenu(mSettings, mpDemo->mpMenuBarCallback, mpDemo->mpDemoName, mCtx.menuBarHeight);
    if (selectedMode != std::nullopt)
    {
        loadDemo(*selectedMode);
    }

    mpDemo->renderUi();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}