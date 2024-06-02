module;

#define NOMINMAX
#include <Windows.h>
#include <cmath>

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

using namespace std::chrono;
using namespace DirectX;
using namespace Microsoft::WRL;
namespace fs = std::filesystem;

export inline const char* interationsNames[] = {"ROTATE", "MOVE", "SCALE", "SELECT"};

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
    };

    struct Settings
    {
        bool isVsync = true;
    } mSettings{};

private:
    void processInput(IWindow::Message msg, float dt);
    void renderUi();
    void loadDemo(App::Demo mode);

    const ParsedOptions mOptions;
    Camera mCamera{0.0f, 0.5f, -10.0f};
    XMVECTOR mCursorPos{};

    std::shared_ptr<IWindow> mpWindow;
    DX11Renderer* mpRenderer;

    enum class InteractionType
    {
        ROTATE,
        MOVE,
        SCALE,
        SELECT,
    } mInteractionType{};

    bool mIsRunning{};
    bool mIsMenuEnabled{};
    bool mIsUiClicked{};
    bool mIsLeftMouseClicked{};
    bool mIsRightMouseClicked{};
    bool mIsCtrlClicked{};
    bool mIsMenuHovered{};

    std::optional<int> mLastXMousePosition;
    std::optional<int> mLastYMousePosition;

    IRenderable::Id mLastSelectedRenderableId = IRenderable::invalidId;

    XMMATRIX mViewMtx = XMMatrixIdentity();
    XMMATRIX mProjMtx = XMMatrixIdentity();

    XMVECTOR mPivotPos{};
    float mPivotPitch{};
    float mPivotYaw{};
    float mPivotRoll{};
    float mPivotScale{};

    bool mIsShiftPressed{};
    bool mIsCameraInMovement{};

    Demo mMode{};

    std::unique_ptr<IDemo> mpDemo;

    std::unordered_set<IRenderable::Id> mSelectedRenderableIds;

    LightsCB mLights{.pos = {{-10.0f, 5.0f, 0.0f, 1.0f}, {10.0f, 5.0f, 0.0f, 1.0f}}};
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

    mpDemo = std::make_unique<DuckDemo>(mpRenderer);
    mpDemo->init();

    mpRenderer->createCB(mLights);
}

void App::run()
{
    mIsRunning = true;

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER previousTime;
    QueryPerformanceCounter(&previousTime);

#ifndef NDEBUG
    auto lastShadersCompilationTime = fs::file_time_type::clock::now();
#endif

    while (mIsRunning)
    {
#ifndef NDEBUG
        bool shouldRecompileShaders = false;
        for (const auto& entry : fs::recursive_directory_iterator(SHADERS_PATH))
        {
            if (fs::is_regular_file(entry.status()))
            {
                const auto lastModified = fs::last_write_time(entry);
                if (lastModified > lastShadersCompilationTime)
                {
                    shouldRecompileShaders = true;
                    lastShadersCompilationTime = fs::file_time_type::clock::now();
                }
            }
        }

        if (shouldRecompileShaders)
        {
            WARN("Starting recompiling shaders!");
            mpRenderer->createShaders();
            WARN("Shaders recompiled!");
        }
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
    if (mIsCameraInMovement)
    {
        mCamera.moveCamera(Camera::FORWARD, dt);
    }

    mpDemo->update(dt);
}

void App::draw()
{
    ID3D11DeviceContext* const pContext = mpRenderer->getContext();

    pContext->VSSetShader(nullptr, nullptr, 0);
    pContext->HSSetShader(nullptr, nullptr, 0);
    pContext->DSSetShader(nullptr, nullptr, 0);
    pContext->GSSetShader(nullptr, nullptr, 0);
    pContext->PSSetShader(nullptr, nullptr, 0);

    mpRenderer->buildGeometryBuffers();

    pContext->ClearRenderTargetView(mpRenderer->getRenderTargetView(), reinterpret_cast<const float*>(&backgroundColor));
    pContext->ClearDepthStencilView(mpRenderer->getDepthStencilView(),
                                                             D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    const XMMATRIX identity = XMMatrixIdentity();
    mViewMtx = mCamera.getViewMatrix();
    mProjMtx = XMMatrixPerspectiveFovLH(XMConvertToRadians(mCamera.getZoom()), mpWindow->getAspectRatio(), 0.01f, 100.0f);

    GlobalCB cb
    {
        .model = identity,
        .view = mViewMtx,
        .invView = XMMatrixInverse(nullptr, mViewMtx),
        .proj = mProjMtx,
        .invProj = XMMatrixInverse(nullptr, mProjMtx),
        .texMtx = XMMatrixIdentity(),
        .cameraPos = mCamera.getPos(),
        .screenWidth = mpWindow->getWidth(),
        .screenHeight = mpWindow->getHeight(),
    };
    mpRenderer->createCB(cb);

    pContext->VSSetConstantBuffers(0, 1, cb.buffer.GetAddressOf());
    pContext->HSSetConstantBuffers(0, 1, cb.buffer.GetAddressOf());
    pContext->DSSetConstantBuffers(0, 1, cb.buffer.GetAddressOf());
    pContext->GSSetConstantBuffers(0, 1, cb.buffer.GetAddressOf());
    pContext->PSSetConstantBuffers(0, 1, cb.buffer.GetAddressOf());

    pContext->IASetInputLayout(mpRenderer->getDefaultInputLayout());

    mpDemo->drawSurface(cb);
    pContext->ClearDepthStencilView(mpRenderer->getDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    mpDemo->draw(cb);
#pragma region cursor
    const XMMATRIX translationMat = XMMatrixTranslationFromVector(mCursorPos);
    cb.model = translationMat;

    mpRenderer->updateCB(cb);

    pContext->VSSetShader(mpRenderer->getShaders().pCursorVS.first.Get(), nullptr, 0);
    pContext->GSSetShader(mpRenderer->getShaders().pCursorGS.first.Get(), nullptr, 0);
    pContext->PSSetShader(mpRenderer->getShaders().pCursorPS.first.Get(), nullptr, 0);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    pContext->Draw(6, 0);
    pContext->GSSetShader(nullptr, nullptr, 0);
#pragma endregion

    for (const auto [renderableIdx, pRenderable] : std::views::enumerate(mpRenderer->mRenderables))
    {
        if (pRenderable == nullptr)
        {
            continue;
        }

        XMMATRIX model = XMMatrixIdentity();

        XMMATRIX localScaleMat = XMMatrixScaling(pRenderable->mScale, pRenderable->mScale, pRenderable->mScale);
        XMMATRIX pivotScaleMat = XMMatrixIdentity();

        XMMATRIX pitchRotationMatrix = XMMatrixRotationX(pRenderable->mPitch);
        XMMATRIX yawRotationMatrix = XMMatrixRotationY(pRenderable->mYaw);
        XMMATRIX rollRotationMatrix = XMMatrixRotationZ(pRenderable->mRoll);

        XMMATRIX localRotationMat = pitchRotationMatrix * yawRotationMatrix * rollRotationMatrix;
        XMMATRIX pivotRotationMat = XMMatrixIdentity();

        XMMATRIX translationToOrigin = XMMatrixIdentity();
        XMMATRIX translationBack = XMMatrixIdentity();

        if (mSelectedRenderableIds.contains(pRenderable->mId))
        {
            XMMATRIX pivotPitchRotationMat = XMMatrixRotationX(mPivotPitch);
            XMMATRIX pivotYawRotationMat = XMMatrixRotationY(mPivotYaw);
            XMMATRIX pivotRollRotationMat = XMMatrixRotationZ(mPivotRoll);

            pivotRotationMat = pivotPitchRotationMat * pivotYawRotationMat * pivotRollRotationMat;

            pivotScaleMat = XMMatrixScaling(mPivotScale, mPivotScale, mPivotScale);

            translationToOrigin = XMMatrixTranslationFromVector(-(mPivotPos - pRenderable->mWorldPos));
            translationBack = XMMatrixTranslationFromVector((mPivotPos - pRenderable->mWorldPos));
        }

        XMMATRIX worldTranslation = XMMatrixTranslationFromVector(pRenderable->mWorldPos);

// clang-format off
        model = localScaleMat * localRotationMat *
                translationToOrigin * pivotScaleMat * pivotRotationMat * translationBack *
                worldTranslation;
// clang-format on

        cb.model = model;

        cb.flags = (std::ranges::find(mSelectedRenderableIds, pRenderable->mId) != mSelectedRenderableIds.end()) ? 1 : 0;

        mpRenderer->updateCB(cb);

        UINT vStride;
        UINT offset{};
        if (dynamic_cast<IBezier*>(pRenderable.get()) == nullptr)
        {
            vStride = sizeof(XMFLOAT3);
        }
        else
        {
            vStride = 4 * sizeof(XMFLOAT3);
        }

        if (dynamic_cast<IBezier*>(pRenderable.get()) == nullptr)
        {
            pContext->IASetInputLayout(mpRenderer->getDefaultInputLayout());
            pContext->VSSetShader(mpRenderer->getShaders().pDefaultVS.first.Get(), nullptr, 0);
            pContext->HSSetShader(nullptr, nullptr, 0);
            pContext->DSSetShader(nullptr, nullptr, 0);
            pContext->GSSetShader(nullptr, nullptr, 0);
            pContext->PSSetShader(mpRenderer->getShaders().pDefaultPS.first.Get(), nullptr, 0);
        }
        else
        {
            pContext->IASetInputLayout(mpRenderer->getBezierInputLayout());
            pContext->VSSetShader(mpRenderer->getShaders().pBezierC0VS.first.Get(), nullptr, 0);
            pContext->HSSetShader(mpRenderer->getShaders().pBezierC0HS.first.Get(), nullptr, 0);
            pContext->DSSetShader(mpRenderer->getShaders().pBezierC0DS.first.Get(), nullptr, 0);
            pContext->GSSetShader(nullptr, nullptr, 0);
            pContext->PSSetShader(mpRenderer->getShaders().pBezierC0PS.first.Get(), nullptr, 0);
        }

        pContext->IASetVertexBuffers(0, 1, mpRenderer->mVertexBuffers.at(renderableIdx).GetAddressOf(), &vStride,
                                     &offset);
        const ComPtr<ID3D11Buffer> indexBuffer = mpRenderer->mIndexBuffers.at(renderableIdx);
        if (indexBuffer != nullptr)
        {
            pContext->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        }

        if (dynamic_cast<Point*>(pRenderable.get()) != nullptr)
        {
            pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        }
        else if (dynamic_cast<IBezier*>(pRenderable.get()) != nullptr)
        {
            pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
        }
        else
        {
            pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        }

        if (auto pBezier = dynamic_cast<IBezier*>(pRenderable.get()); pBezier == nullptr)
        {
            pContext->DrawIndexed(static_cast<UINT>(pRenderable->getTopology().size()), 0, 0);
        }
        else
        {
            const auto& controlPoints = pBezier->mControlPointRenderableIds;
            const auto controlPointsSize = controlPoints.size();
            for (unsigned i = 0; i < controlPointsSize + 1; i += IBezier::controlPointsNumber)
            {
                offset = i * sizeof(XMFLOAT3);
                pContext->IASetVertexBuffers(0, 1, mpRenderer->mVertexBuffers.at(renderableIdx).GetAddressOf(),
                                             &vStride,
                                             &offset);

                pContext->Draw(1, 0);

                if (pBezier->mIsPolygon)
                {
                    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
                    pContext->HSSetShader(nullptr, nullptr, 0);
                    pContext->DSSetShader(nullptr, nullptr, 0);
                    pContext->GSSetShader(mpRenderer->getShaders().pBezierC0BorderGS.first.Get(), nullptr, 0);
                    int tmpFlags = cb.flags;
                    cb.flags = 2;

                    mpRenderer->updateCB(cb);

                    pContext->Draw(1, 0);

                    cb.flags = tmpFlags;
                    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
                    pContext->HSSetShader(mpRenderer->getShaders().pBezierC0HS.first.Get(), nullptr, 0);
                    pContext->DSSetShader(mpRenderer->getShaders().pBezierC0DS.first.Get(), nullptr, 0);
                    pContext->GSSetShader(nullptr, nullptr, 0);

                    mpRenderer->updateCB(cb);
                }
            }
        }
    }

    if (mSelectedRenderableIds.size() > 1)
    {
        XMVECTOR sum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        for (const auto& selectedRenderableId : mSelectedRenderableIds)
        {
            const auto pRenderable = mpRenderer->getRenderable(selectedRenderableId);
            sum = XMVectorAdd(sum, pRenderable->mLocalPos + pRenderable->mWorldPos);
        }

        float numPositions = static_cast<float>(mSelectedRenderableIds.size());
        mPivotPos = XMVectorScale(sum, 1.0f / numPositions);

        auto pPoint = std::make_unique<Point>(mPivotPos, 0.1f, 25, false);
        pPoint->regenerateData();
        IRenderable::Id mId = pPoint->mId;
        mpRenderer->addRenderable(std::move(pPoint));
        pPoint.reset();

        mpRenderer->buildGeometryBuffers();

        const XMMATRIX translationMat = XMMatrixTranslationFromVector(mPivotPos);
        cb.model = translationMat;
        cb.flags = 2;

        mpRenderer->updateCB(cb);

        UINT vStride = sizeof(XMFLOAT3), offset = 0;
        pContext->IASetVertexBuffers(0, 1, &mpRenderer->mVertexBuffers.back(), &vStride, &offset);
        pContext->IASetIndexBuffer(mpRenderer->mIndexBuffers.back().Get(), DXGI_FORMAT_R32_UINT, 0);

        pContext->VSSetShader(mpRenderer->getShaders().pDefaultVS.first.Get(), nullptr, 0);
        pContext->PSSetShader(mpRenderer->getShaders().pDefaultPS.first.Get(), nullptr, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        auto pTest = mpRenderer->getRenderable(mId);
        pContext->DrawIndexed(static_cast<UINT>(pTest->getTopology().size()), 0, 0);

        mpRenderer->removeRenderable(mId);
    }

    renderUi();

    mpRenderer->getSwapchain()->Present(mSettings.isVsync, 0);
}

void App::processInput(IWindow::Message msg, float dt)
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
    case IWindow::Message::KEY_DELETE_DOWN:
        for (auto it = mSelectedRenderableIds.begin(); it != mSelectedRenderableIds.end(); it = mSelectedRenderableIds.begin())
        {
            mpRenderer->removeRenderable(*it);
            mSelectedRenderableIds.erase(*it);
        }
        for (auto& pRenderable : mpRenderer->mRenderables)
        {
            pRenderable->regenerateData();
        }
        break;
    case IWindow::Message::KEY_W_DOWN:
        if (!mIsUiClicked)
        {
            mIsCameraInMovement = true;
        }
        break;
    case IWindow::Message::KEY_W_UP:
        if (!mIsUiClicked)
        {
            mIsCameraInMovement = false;
        }
        break;
    case IWindow::Message::KEY_S_DOWN:
        if (!mIsUiClicked)
        {
            mCamera.moveCamera(Camera::BACKWARD, dt);
        }
        break;
    case IWindow::Message::KEY_A_DOWN:
        if (!mIsUiClicked)
        {
            mCamera.moveCamera(Camera::LEFT, dt);
        }
        break;
    case IWindow::Message::KEY_D_DOWN:
        if (!mIsUiClicked)
        {
            mCamera.moveCamera(Camera::RIGHT, dt);
        }
        break;
    case IWindow::Message::KEY_CTRL_DOWN:
        mIsCtrlClicked = true;
        break;
    case IWindow::Message::KEY_CTRL_UP:
        mIsCtrlClicked = false;
        break;
    case IWindow::Message::MOUSE_LEFT_UP:
        mIsLeftMouseClicked = false;
        break;
    case IWindow::Message::MOUSE_RIGHT_DOWN:
        mIsRightMouseClicked = true;
        break;
    case IWindow::Message::MOUSE_RIGHT_UP:
        mIsRightMouseClicked = false;
        break;
    case IWindow::Message::KEY_SHIFT_DOWN:
        mIsShiftPressed = true;
        break;
    case IWindow::Message::KEY_SHIFT_UP:
        mIsShiftPressed = false;
        break;
    case IWindow::Message::MOUSE_WHEEL:
        if (!mIsMenuHovered)
        {
            const auto mouseWheel = mpWindow->getEventData<IWindow::Event::MouseWheel>();
            mCamera.setZoom(static_cast<float>(mouseWheel.yOffset));
        }
        break;
    case IWindow::Message::MOUSE_LEFT_DOWN: {
        mIsLeftMouseClicked = true;

        if (mInteractionType == InteractionType::SELECT)
        {
            const auto eventData = mpWindow->getEventData<IWindow::Event::MousePosition>();

            float mouseX = static_cast<float>(eventData.x);
            float mouseY = static_cast<float>(eventData.y);
            float x = (2.0f * mouseX) / mpWindow->getWidth() - 1.0f;
            float y = 1.0f - (2.0f * mouseY) / mpWindow->getHeight();
            float z = 1.0f;

            XMVECTOR rayNDC = XMVectorSet(x, y, z, 1.0f);

            XMMATRIX invProjMtx = XMMatrixInverse(nullptr, mProjMtx);
            XMMATRIX invViewMtx = XMMatrixInverse(nullptr, mViewMtx);

            XMVECTOR rayClip = XMVectorSet(x, y, -1.0f, 1.0f);
            XMVECTOR rayView = XMVector4Transform(rayClip, invProjMtx);

            rayView = XMVectorSetW(rayView, 0.0f);

            XMVECTOR rayWorld = XMVector4Transform(rayView, invViewMtx);
            rayWorld = XMVector3Normalize(rayWorld);

            XMVECTOR rayOrigin = mCamera.getPos();
            XMVECTOR rayDir = rayWorld;

            for (const auto& pRenderable : mpRenderer->mRenderables)
            {
                const auto pPoint = dynamic_cast<Point*>(pRenderable.get());
                if (pPoint == nullptr)
                {
                    continue;
                }

                const bool intersection =
                    mg::rayIntersectsSphere(rayOrigin, rayDir, pRenderable->mWorldPos, pPoint->mRadius);

                if (intersection)
                {
                    mSelectedRenderableIds.insert(pRenderable->mId);
                }
            }
        }
    }
    break;
    case IWindow::Message::MOUSE_MIDDLE_DOWN:
        const auto eventData = mpWindow->getEventData<IWindow::Event::MousePosition>();
        mCursorPos = mCamera.getPos() + mCamera.getFront() * 4.0f;

        for (auto& pRenderable : mpRenderer->mRenderables)
        {
            pRenderable->mLocalPos = pRenderable->mWorldPos - mCursorPos;
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

        if (mIsLeftMouseClicked && (!mIsUiClicked) && !(mSelectedRenderableIds.empty()))
        {
            const float pitchOffset = yOffset * 0.2f;
            const float yawOffset = xOffset * 0.2f;

            if ((mSelectedRenderableIds.size() > 1) && (mInteractionType == InteractionType::ROTATE))
            {
                mPivotPitch += pitchOffset;
                mPivotYaw += yawOffset;
                break;
            }

            if ((mSelectedRenderableIds.size() > 1) && (mInteractionType == InteractionType::SCALE))
            {
                mPivotScale += -yOffset * 0.1f;
                break;
            }

            for (const IRenderable::Id selectedRenderableId : mSelectedRenderableIds)
            {
                IRenderable* const pRenderable = mpRenderer->getRenderable(selectedRenderableId);

                switch (mInteractionType)
                {
                case InteractionType::ROTATE:
                    pRenderable->mPitch += pitchOffset;
                    pRenderable->mYaw += yawOffset;
                    break;
                case InteractionType::MOVE:
                    pRenderable->mWorldPos += XMVectorSet(xOffset * 0.1f, yOffset * 0.1f, 0.0f, 0.0f);
                    break;
                case InteractionType::SCALE:
                    pRenderable->mScale += -yOffset * 0.1f;
                    break;
                }

                const bool isBezierSelected = dynamic_cast<IBezier*>(pRenderable) != nullptr;

                for (const auto& pRenderable : mpRenderer->mRenderables)
                {
                    if (auto pBezier = dynamic_cast<IBezier*>(pRenderable.get()); pBezier != nullptr)
                    {
                        const auto& controlPointIds = pBezier->mControlPointRenderableIds;
                        for (const IRenderable::Id controlPointRenderableId : controlPointIds)
                        {
                            IRenderable* const pControlPointRenderable =
                                mpRenderer->getRenderable(controlPointRenderableId);

                            if (isBezierSelected)
                            {
                                pControlPointRenderable->mWorldPos +=
                                    XMVectorSet(xOffset * 0.1f, yOffset * 0.1f, 0.0f, 0.0f);
                            }
                        }

                        pBezier->generateGeometry();
                    }
                }
            }
        }
        else if (mIsRightMouseClicked && (!mIsUiClicked))
        {
            mCamera.rotateCamera(xOffset, yOffset);
        }
    }
    break;
    };

    mpDemo->processInput(msg, dt);
}

void App::loadDemo(App::Demo mode)
{
    mMode = mode;

    mpRenderer->mRenderables.clear();

    switch (mMode)
    {
    case App::Demo::CAD:
        mpDemo = std::make_unique<CADDemo>(mpRenderer);
        break;
    case App::Demo::DUCK:
        mpDemo = std::make_unique<DuckDemo>(mpRenderer);
        break;
    default:
        break;
    }

    mpDemo->init();
}