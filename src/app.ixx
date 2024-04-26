module;

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

using namespace std::chrono;
using namespace DirectX;
using namespace Microsoft::WRL;
namespace fs = std::filesystem;

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

    void updateScene(float dt);
    void renderScene();

private:
    void processInput(IWindow::Message msg, float deltaTime);
    void renderUi();

    const ParsedOptions mOptions;
    Camera mCamera{0.0f, 0.5f, -10.0f};
    XMVECTOR mCursorPos{};

    std::shared_ptr<IWindow> mpWindow;
    std::unique_ptr<DX11Renderer> mpRenderer;

    enum class InteractionType
    {
        ROTATE,
        MOVE,
        SCALE,
        SELECT,
    } mInteractionType{};
    inline static const char* interationsNames[] = {"ROTATE", "MOVE", "SCALE", "SELECT"};

    bool mIsRunning{};
    bool mIsMenuEnabled{};
    bool mIsUiClicked{};
    bool mIsLeftMouseClicked{};
    bool mIsRightMouseClicked{};
    bool mIsCtrlClicked{};

    std::optional<int> mLastXMousePosition;
    std::optional<int> mLastYMousePosition;

    std::unordered_set<IRenderable::Id> mSelectedRenderableIds;

    XMMATRIX mView = XMMatrixIdentity();
    XMMATRIX mProj = XMMatrixIdentity();

    XMVECTOR mPivotPos{};
    float mPivotPitch{};
    float mPivotYaw{};
    float mPivotRoll{};
    float mPivotScale{};
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

    std::vector<unsigned> selectedPointsIds;

    XMVECTOR pos{-2.f, 2.f, 0};
    auto pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.push_back(pPoint->id);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{2.f, 2.f, 0};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.push_back(pPoint->id);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{0.f, 0.f, 0};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.push_back(pPoint->id);
    mpRenderer->addRenderable(std::move(pPoint));

    auto pBezier = std::make_unique<BezierC0>(selectedPointsIds, mpRenderer.get());
    pBezier->regenerateData();
    mpRenderer->addRenderable(std::move(pBezier));
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
    const fs::path shaderPaths{ASSETS_PATH "shaders/"};
#endif

    while (mIsRunning)
    {
#ifndef NDEBUG
        bool shouldRecompileShaders = false;
        for (const auto& entry : fs::recursive_directory_iterator(shaderPaths))
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
        float deltaTime = static_cast<float>(elapsedTime);

        previousTime = nowTime;

        IWindow::Message msg = mpWindow->getMessage();
        while (msg != IWindow::Message::EMPTY)
        {
            processInput(msg, deltaTime);
            msg = mpWindow->getMessage();
        }

        updateScene(deltaTime);
        renderScene();
    }
}

void App::updateScene(float dt)
{

}

void App::renderScene()
{
    mpRenderer->buildGeometryBuffers();

    ID3D11DeviceContext* const pContext = mpRenderer->getContext();

    pContext->ClearRenderTargetView(mpRenderer->getRenderTargetView(), reinterpret_cast<const float*>(&backgroundColor));
    pContext->ClearDepthStencilView(mpRenderer->getDepthStencilView(),
                                                             D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    const XMMATRIX identity = XMMatrixIdentity();
    mView = mCamera.getViewMatrix();
    mProj = XMMatrixPerspectiveFovLH(XMConvertToRadians(mCamera.getZoom()), mpWindow->getAspectRatio(), 1.0f, 1000.0f);

    ConstantBufferData data
    {
        .model = identity,
        .view = mView,
        .invView = XMMatrixInverse(nullptr, mView),
        .proj = mProj,
        .invProj = XMMatrixInverse(nullptr, mProj),
    };

    D3D11_MAPPED_SUBRESOURCE cbData;
    pContext->Map(mpRenderer->getConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
    memcpy(cbData.pData, &data, sizeof(data));
    pContext->Unmap(mpRenderer->getConstantBuffer(), 0);

    pContext->VSSetConstantBuffers(0, 1, mpRenderer->getAddressOfConstantBuffer());
    pContext->HSSetConstantBuffers(0, 1, mpRenderer->getAddressOfConstantBuffer());
    pContext->DSSetConstantBuffers(0, 1, mpRenderer->getAddressOfConstantBuffer());
    pContext->GSSetConstantBuffers(0, 1, mpRenderer->getAddressOfConstantBuffer());

    pContext->IASetInputLayout(mpRenderer->getDefaultInputLayout());

    {
        pContext->VSSetShader(mpRenderer->mpGridVS.Get(), nullptr, 0);
        pContext->PSSetShader(mpRenderer->mpGridPS.Get(), nullptr, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pContext->Draw(6, 0);
    }
    pContext->ClearDepthStencilView(mpRenderer->getDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    {
        const XMMATRIX translationMat = XMMatrixTranslationFromVector(mCursorPos);
        data.model = translationMat;

        pContext->Map(mpRenderer->getConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
        memcpy(cbData.pData, &data, sizeof(data));
        pContext->Unmap(mpRenderer->getConstantBuffer(), 0);

        pContext->VSSetShader(mpRenderer->mpCursorVS.Get(), nullptr, 0);
        pContext->GSSetShader(mpRenderer->mpCursorGS.Get(), nullptr, 0);
        pContext->PSSetShader(mpRenderer->mpCursorPS.Get(), nullptr, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        pContext->Draw(6, 0);
        pContext->GSSetShader(nullptr, nullptr, 0);
    }

    for (auto const [i, pRenderable] : std::views::enumerate(mpRenderer->mRenderables))
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

        if (mSelectedRenderableIds.contains(pRenderable->id))
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

        data.model = model;

        data.flags = (std::ranges::find(mSelectedRenderableIds, pRenderable->id) != mSelectedRenderableIds.end()) ? 1 : 0;

        pContext->Map(mpRenderer->getConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
        memcpy(cbData.pData, &data, sizeof(data));
        pContext->Unmap(mpRenderer->getConstantBuffer(), 0);

        UINT vStride;
        const UINT offset{};
        if (dynamic_cast<Bezier*>(pRenderable.get()) == nullptr)
        {
            vStride = sizeof(XMFLOAT3);
        }
        else
        {
            vStride = 4 * sizeof(XMFLOAT3);
        }

        if (dynamic_cast<Bezier*>(pRenderable.get()) == nullptr)
        {
            pContext->IASetInputLayout(mpRenderer->getDefaultInputLayout());
            pContext->VSSetShader(mpRenderer->mpVS.Get(), nullptr, 0);
            pContext->HSSetShader(nullptr, nullptr, 0);
            pContext->DSSetShader(nullptr, nullptr, 0);
            pContext->PSSetShader(mpRenderer->mpPS.Get(), nullptr, 0);
        }
        else
        {
            pContext->IASetInputLayout(mpRenderer->getBezierInputLayout());
            pContext->VSSetShader(mpRenderer->mpBezierC0VS.Get(), nullptr, 0);
            pContext->HSSetShader(mpRenderer->mpBezierC0HS.Get(), nullptr, 0);
            pContext->DSSetShader(mpRenderer->mpBezierC0DS.Get(), nullptr, 0);
            pContext->PSSetShader(mpRenderer->mpBezierC0PS.Get(), nullptr, 0);
        }

        pContext->IASetVertexBuffers(0, 1, mpRenderer->mVertexBuffers.at(i).GetAddressOf(), &vStride, &offset);
        const ComPtr<ID3D11Buffer> indexBuffer = mpRenderer->mIndexBuffers.at(i);
        if (indexBuffer != nullptr)
        {
            pContext->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        }

        if (dynamic_cast<Point*>(pRenderable.get()) != nullptr)
        {
            pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        }
        else if (dynamic_cast<Bezier*>(pRenderable.get()) != nullptr)
        {
            pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
        }
        else
        {
            pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        }

        if (dynamic_cast<Bezier*>(pRenderable.get()) == nullptr)
        {
            pContext->DrawIndexed(static_cast<UINT>(pRenderable->getTopology().size()), 0, 0);
        }
        else
        {
            pContext->Draw(1, 0);
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
        IRenderable::Id id = pPoint->id;
        mpRenderer->addRenderable(std::move(pPoint));
        pPoint.reset();

        mpRenderer->buildGeometryBuffers();

        const XMMATRIX translationMat = XMMatrixTranslationFromVector(mPivotPos);
        data.model = translationMat;
        data.flags = 2;

        pContext->Map(mpRenderer->getConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
        memcpy(cbData.pData, &data, sizeof(data));
        pContext->Unmap(mpRenderer->getConstantBuffer(), 0);

        UINT vStride = sizeof(XMFLOAT3), offset = 0;
        pContext->IASetVertexBuffers(0, 1, mpRenderer->mVertexBuffers.back().GetAddressOf(), &vStride, &offset);
        pContext->IASetIndexBuffer(mpRenderer->mIndexBuffers.back().Get(), DXGI_FORMAT_R32_UINT, 0);

        pContext->VSSetShader(mpRenderer->mpVS.Get(), nullptr, 0);
        pContext->PSSetShader(mpRenderer->mpPS.Get(), nullptr, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        auto pTest = mpRenderer->getRenderable(id);
        pContext->DrawIndexed(static_cast<UINT>(pTest->getTopology().size()), 0, 0);

        mpRenderer->removeRenderable(id);
    }

    renderUi();

    mpRenderer->getSwapchain()->Present(1, 0);
}

void App::processInput(IWindow::Message msg, float deltaTime)
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
        break;
    case IWindow::Message::KEY_W_DOWN:
        if (!mIsUiClicked)
        {
            mCamera.moveCamera(Camera::FORWARD, deltaTime);
        }
        break;
    case IWindow::Message::KEY_S_DOWN:
        if (!mIsUiClicked)
        {
            mCamera.moveCamera(Camera::BACKWARD, deltaTime);
        }
        break;
    case IWindow::Message::KEY_A_DOWN:
        if (!mIsUiClicked)
        {
            mCamera.moveCamera(Camera::LEFT, deltaTime);
        }
        break;
    case IWindow::Message::KEY_D_DOWN:
        if (!mIsUiClicked)
        {
            mCamera.moveCamera(Camera::RIGHT, deltaTime);
        }
        break;
    case IWindow::Message::KEY_CTRL_DOWN:
        mIsCtrlClicked = true;
        break;
    case IWindow::Message::KEY_CTRL_UP:
        mIsCtrlClicked = false;
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

                XMMATRIX projMatrix = mProj;
                XMMATRIX viewMatrix = mView;
                XMMATRIX invProjMatrix = XMMatrixInverse(nullptr, projMatrix);
                XMMATRIX invViewMatrix = XMMatrixInverse(nullptr, viewMatrix);

                XMVECTOR rayClip = XMVectorSet(x, y, -1.0f, 1.0f);
                XMVECTOR rayView = XMVector4Transform(
                    rayClip, invProjMatrix);

                rayView = XMVectorSetW(rayView, 0.0f);

                XMVECTOR rayWorld = XMVector4Transform(rayView, invViewMatrix);
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

                    const bool intersection = mg::rayIntersectsSphere(rayOrigin, rayDir, pRenderable->mWorldPos, pPoint->mRadius);

                    if (intersection)
                    {
                        mSelectedRenderableIds.insert(pRenderable->id);
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
    case IWindow::Message::MOUSE_LEFT_UP:
        mIsLeftMouseClicked = false;
        break;
    case IWindow::Message::MOUSE_RIGHT_DOWN:
        mIsRightMouseClicked = true;
        break;
    case IWindow::Message::MOUSE_RIGHT_UP:
        mIsRightMouseClicked = false;
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

            for (const auto selectedRenderableId : mSelectedRenderableIds)
            {
                const auto pRenderable = mpRenderer->getRenderable(selectedRenderableId);

                switch (mInteractionType)
                {
                case InteractionType::ROTATE:
                    pRenderable->mPitch += pitchOffset;
                    pRenderable->mYaw   += yawOffset;
                    break;
                case InteractionType::MOVE:
                    pRenderable->mWorldPos += XMVectorSet(xOffset * 0.1f, yOffset * 0.1f, 0.0f, 0.0f);
                    break;
                case InteractionType::SCALE:
                    pRenderable->mScale += -yOffset * 0.1f;
                    break;
                }
            }
        }
        else if (mIsRightMouseClicked && (!mIsUiClicked))
        {
            mCamera.rotateCamera(xOffset, yOffset);
        }
        }
        break;
    case IWindow::Message::MOUSE_WHEEL:
        const auto mouseWheel = mpWindow->getEventData<IWindow::Event::MouseWheel>();
        mCamera.setZoom(static_cast<float>(mouseWheel.yOffset));
        break;
    };
}

void App::renderUi()
{

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    mIsMenuEnabled = false;
    mIsUiClicked = false;

    const float menuWidth = mpWindow->getWidth() * 0.2f;

    ImGui::SetNextWindowPos(ImVec2{mpWindow->getWidth() - menuWidth, 0});
    ImGui::SetNextWindowSize(ImVec2{menuWidth, static_cast<float>(mpWindow->getHeight())});

    if (ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        mIsMenuEnabled = true;
    }

    ImGui::Text("CAD: ");
    ImGui::Spacing();

    ImGui::Combo("##interactionType", reinterpret_cast<int*>(&mInteractionType), interationsNames,
                 IM_ARRAYSIZE(interationsNames));
    mIsUiClicked |= ImGui::IsItemActive();

    ImGui::Spacing();

    if (ImGui::Button("Add Torus"))
    {
        auto pTorus = std::make_unique<Torus>(mCursorPos);
        pTorus->regenerateData();
        mpRenderer->addRenderable(std::move(pTorus));
    }
    ImGui::SameLine();

    if (ImGui::Button("Add Point"))
    {
        auto pPoint = std::make_unique<Point>(mCursorPos);
        pPoint->regenerateData();
        mpRenderer->addRenderable(std::move(pPoint));
    }

    int newSelectedItemId = -1;

    ImGui::Separator();
    ImGui::Spacing();

    ImGuiStyle& style = ImGui::GetStyle();

    ImVec4 backgroundColor = style.Colors[ImGuiCol_WindowBg];
    backgroundColor.z *= 2.2f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, backgroundColor);
    const float renderablesHeight = mpWindow->getHeight() * 0.2f;

    ImGui::Text("Renderables:");
    ImGui::BeginChild("Scrolling", ImVec2{menuWidth * 0.9f, renderablesHeight});

    for (int i = 0; i < mpRenderer->mRenderables.size(); ++i)
    {
        const auto& pRenderable = mpRenderer->mRenderables.at(i);

        if (ImGui::Selectable(pRenderable->mTag.c_str(), mSelectedRenderableIds.contains(pRenderable->id)))
        {
            newSelectedItemId = i;
            mIsUiClicked |= ImGui::IsItemActive();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    bool isNewItemSelected{};
    if (newSelectedItemId != -1)
    {
        int i = 0;
        for (const auto renderableSelectedId : mSelectedRenderableIds)
        {
            IRenderable* const pRenderable = mpRenderer->getRenderable(renderableSelectedId);

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

            if (mSelectedRenderableIds.contains(pRenderable->id))
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

            pRenderable->mWorldPos = model.r[3];

            const XMMATRIX scaleMat = localScaleMat * pivotScaleMat;
            pRenderable->mScale = XMVectorGetX(scaleMat.r[0]);

            const XMMATRIX rotationMat = localRotationMat * translationToOrigin * pivotRotationMat * translationBack;
            const auto [pitch, yaw, roll] = mg::getPitchYawRollFromRotationMat(rotationMat);

            pRenderable->mPitch = pitch;
            pRenderable->mYaw = yaw;
            pRenderable->mRoll = roll;
        }

        mPivotPitch = 0;
        mPivotYaw = 0;
        mPivotRoll = 0;

        mPivotScale = 1;

        const auto newId = mpRenderer->mRenderables.at(newSelectedItemId)->id;
        if (!mSelectedRenderableIds.contains(newId))
        {
            if (!mIsCtrlClicked)
            {
                mSelectedRenderableIds.clear();
            }

            mSelectedRenderableIds.insert(newId);
            isNewItemSelected = true;
        }
        else
        {
            mSelectedRenderableIds.erase(newId);
        }
    }

    if (mSelectedRenderableIds.size() > 0)
    {
        ImGui::Separator();
        ImGui::Spacing();
    }

    if (mSelectedRenderableIds.size() == 1)
    {
        const IRenderable::Id selectedRenderableId = *mSelectedRenderableIds.begin();

        bool dataChanged{};
        IRenderable* pSelectedRenderable = mpRenderer->getRenderable(selectedRenderableId);

        ImGui::Text("Selected item: %s", pSelectedRenderable->mTag.c_str());

        static char nameBuffer[256]{};

        if (isNewItemSelected)
        {
            strncpy_s(nameBuffer, pSelectedRenderable->mTag.c_str(), sizeof(nameBuffer) - 1);
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        }

        dataChanged |= ImGui::InputText("##tag", nameBuffer, sizeof(nameBuffer));
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::SameLine();

        if (ImGui::Button("Rename"))
        {
            pSelectedRenderable->mTag = nameBuffer;
        }

        float localPos[3]{};
        float worldPos[3]{};
        float rotation[3]{};
        float scale{};

        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(localPos), pSelectedRenderable->mLocalPos);
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(worldPos), pSelectedRenderable->mWorldPos);
        rotation[0] = pSelectedRenderable->mPitch;
        rotation[1] = pSelectedRenderable->mYaw;
        rotation[2] = pSelectedRenderable->mRoll;

        ImGui::Text("Local Position:");
        if (ImGui::InputFloat3("##localPos", localPos))
        {
            pSelectedRenderable->mLocalPos = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(localPos));
            pSelectedRenderable->mWorldPos = pSelectedRenderable->mLocalPos - mCursorPos;
        }
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Text("World Position:");
        if (ImGui::InputFloat3("##worldPos", worldPos))
        {
            pSelectedRenderable->mWorldPos = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(worldPos));
            pSelectedRenderable->mLocalPos = pSelectedRenderable->mWorldPos - mCursorPos;
        }
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Rotation:");
        if (ImGui::InputFloat3("##rotation", rotation))
        {
            pSelectedRenderable->mPitch = rotation[0];
            pSelectedRenderable->mYaw = rotation[1];
            pSelectedRenderable->mRoll = rotation[2];
        }
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Scale:");
        ImGui::InputFloat("##scale", &pSelectedRenderable->mScale);
        mIsUiClicked |= ImGui::IsItemActive();

        if (auto pTorus = dynamic_cast<Torus*>(pSelectedRenderable); pTorus != nullptr)
        {
            ImGui::Text("Major Radius:");
            dataChanged |= ImGui::SliderFloat("##majorRadius", &pTorus->mMajorRadius, 0.0f, 1.0f);
            mIsUiClicked |= ImGui::IsItemActive();

            ImGui::Text("Minor Radius:");
            dataChanged |= ImGui::SliderFloat("##minorRadius", &pTorus->mMinorRadius, 0.0f, 1.0f);
            mIsUiClicked |= ImGui::IsItemActive();

            ImGui::Spacing();

            ImGui::Text("Major Segments:");
            dataChanged |= ImGui::SliderInt("##majorSegments", &pTorus->mMajorSegments, 3, 100);
            mIsUiClicked |= ImGui::IsItemActive();

            ImGui::Text("Minor Segments:");
            dataChanged |= ImGui::SliderInt("##minorSegments", &pTorus->mMinorSegments, 3, 100);
            mIsUiClicked |= ImGui::IsItemActive();
        }
        else if (auto pPoint = dynamic_cast<Point*>(pSelectedRenderable); pPoint != nullptr)
        {
            ImGui::Text("Radius:");
            dataChanged |= ImGui::SliderFloat("##radius", &pPoint->mRadius, 0.1f, 10.0f);
            mIsUiClicked |= ImGui::IsItemActive();

            ImGui::Text("Segments:");
            dataChanged |= ImGui::SliderInt("##segments", &pPoint->mSegments, 1, 100);
            mIsUiClicked |= ImGui::IsItemActive();
        }

        if (dataChanged)
        {
            pSelectedRenderable->regenerateData();
            mpRenderer->buildGeometryBuffers();
        }

        mIsUiClicked |= dataChanged;
    }
    else if (mSelectedRenderableIds.size() > 1)
    {
        std::vector<IRenderable::Id> selectedPointsIds;

        for (const auto id : mSelectedRenderableIds)
        {
            IRenderable* pSelectedRenderable = mpRenderer->getRenderable(id);
            if (dynamic_cast<Point*>(pSelectedRenderable) != nullptr)
            {
                selectedPointsIds.push_back(id);
            }
        }

        if (selectedPointsIds.size() >= 3)
        {
            if (ImGui::Button("Add Bezier C0"))
            {
                auto pBezier = std::make_unique<BezierC0>(selectedPointsIds, mpRenderer.get());
                pBezier->regenerateData();
                mpRenderer->addRenderable(std::move(pBezier));
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Bezier C2"))
            {
                ASSERT(false);
            }
        }

        float pivotPos[3]{};

        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(pivotPos), mPivotPos);

        ImGui::Text("Pivot position:");
        if (ImGui::InputFloat3("##pivotPos", pivotPos))
        {
            mPivotPos = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(pivotPos));
        }
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Pivot Pitch:");
        ImGui::InputFloat("##pivotPitch", &mPivotPitch);
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Pivot Yaw:");
        ImGui::InputFloat("##pivotYaw", &mPivotYaw);
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Pivot Roll:");
        ImGui::InputFloat("##pivotRoll", &mPivotRoll);
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Pivot Scale:");
        ImGui::InputFloat("##pivotScale", &mPivotScale);
        mIsUiClicked |= ImGui::IsItemActive();
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}