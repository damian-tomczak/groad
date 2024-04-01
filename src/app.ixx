module;

#include <Windows.h>
#include <cmath>

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
using namespace DirectX;

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

    std::optional<int> mLastXMousePosition;
    std::optional<int> mLastYMousePosition;

    std::unordered_set<IRenderable::Id> mSelectedRenderables;

    XMMATRIX mView = XMMatrixIdentity();
    XMMATRIX mProj = XMMatrixIdentity();

    XMVECTOR mPivotPos{};
    XMVECTOR mPivotEulerAngles{};
};

module :private;

void PrintXMVECTOR(DirectX::XMVECTOR v)
{
    DirectX::XMFLOAT4 vec;
    DirectX::XMStoreFloat4(&vec, v);

    std::cout << "(" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ")" << std::endl;
}

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
}

void App::run()
{
    mIsRunning = true;

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER previousTime;
    QueryPerformanceCounter(&previousTime);

    while (mIsRunning)
    {
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

        updateScene(dt);
        renderScene();
    }
}

void App::updateScene(float dt)
{
    //if (mInteractionType != InteractionType::ROTATE || mpRenderer->mRenderables.size() < 2)
    //{
    //    return;
    //}

    //for (const auto selectedRenderableId : mSelectedRenderables)
    //{
    //    const auto pRenderable = mpRenderer->getRenderable(selectedRenderableId);

    //    mPivotYaw += 0.5f * dt;
    //}
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
        pContext->PSSetShader(mpRenderer->mpCursorPS.Get(), nullptr, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        pContext->Draw(6, 0);
    }

    for (auto const [i, pRenderable] : std::views::enumerate(mpRenderer->mRenderables))
    {
        if (pRenderable == nullptr)
        {
            continue;
        }

        XMMATRIX model = XMMatrixIdentity();

        XMMATRIX localScaleMat = XMMatrixScalingFromVector(pRenderable->mScale);
        XMMATRIX pivotScaleMat = XMMatrixIdentity();

        XMMATRIX pitchRotationMatrix = XMMatrixRotationX(XMVectorGetX(pRenderable->mEulerAngles));
        XMMATRIX yawRotationMatrix = XMMatrixRotationY(XMVectorGetY(pRenderable->mEulerAngles));
        XMMATRIX rollRotationMatrix = XMMatrixRotationZ(XMVectorGetZ(pRenderable->mEulerAngles));

        XMMATRIX localRotationMat = pitchRotationMatrix * yawRotationMatrix * rollRotationMatrix;
        XMMATRIX pivotRotationMat = XMMatrixIdentity();

        if (mSelectedRenderables.size() > 1)
        {
            XMMATRIX pivotPitchRotationMat = XMMatrixRotationX(XMVectorGetX(mPivotEulerAngles));
            XMMATRIX pivotYawRotationMat = XMMatrixRotationY(XMVectorGetY(mPivotEulerAngles));
            XMMATRIX pivotRollRotationMat = XMMatrixRotationZ(XMVectorGetZ(mPivotEulerAngles));

            pivotRotationMat = pivotPitchRotationMat * pivotYawRotationMat * pivotRollRotationMat;

            pivotScaleMat = XMMatrixScalingFromVector(pRenderable->mScale);
        }

        XMMATRIX worldTranslation = XMMatrixTranslationFromVector(pRenderable->mWorldPos);

        XMMATRIX translationToOrigin = XMMatrixTranslationFromVector(-(mPivotPos - pRenderable->mWorldPos));
        XMMATRIX translationBack = XMMatrixTranslationFromVector((mPivotPos - pRenderable->mWorldPos));

        model = localScaleMat * localRotationMat *
                translationToOrigin * pivotRotationMat * translationBack *
                worldTranslation;

        data.model = model;

        data.flags = (std::ranges::find(mSelectedRenderables, pRenderable->id) != mSelectedRenderables.end()) ? 1 : 0;

        pContext->Map(mpRenderer->getConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
        memcpy(cbData.pData, &data, sizeof(data));
        pContext->Unmap(mpRenderer->getConstantBuffer(), 0);

        const UINT vStride{sizeof(XMFLOAT3)}, offset{};
        pContext->IASetVertexBuffers(0, 1, mpRenderer->mVertexBuffers.at(i).GetAddressOf(), &vStride, &offset);
        pContext->IASetIndexBuffer(mpRenderer->mIndexBuffers.at(i).Get(), DXGI_FORMAT_R32_UINT, 0);
        pContext->IASetInputLayout(mpRenderer->getInputLayout());
        pContext->VSSetShader(mpRenderer->mpVS.Get(), nullptr, 0);
        pContext->PSSetShader(mpRenderer->mpPS.Get(), nullptr, 0);

        if (dynamic_cast<Point*>(pRenderable.get()) == nullptr)
        {
            pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        }
        else
        {
            pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        }

        pContext->DrawIndexed(static_cast<UINT>(pRenderable->getTopology().size()), 0, 0);
    }

    if (mSelectedRenderables.size() > 1)
    {
        XMVECTOR sum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        for (const auto& selectedRenderableId : mSelectedRenderables)
        {
            const auto pRenderable = mpRenderer->getRenderable(selectedRenderableId);
            sum = XMVectorAdd(sum, pRenderable->mLocalPos + pRenderable->mWorldPos);
        }

        float numPositions = static_cast<float>(mSelectedRenderables.size());
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
        mpRenderer->onResize();
        break;
    case IWindow::Message::KEY_DELETE_DOWN:
        for (auto it = mSelectedRenderables.begin(); it != mSelectedRenderables.end(); it = mSelectedRenderables.begin())
        {
            mpRenderer->removeRenderable(*it);
            mSelectedRenderables.erase(*it);
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
                    auto pPoint = dynamic_cast<Point*>(pRenderable.get());
                    if (pPoint == nullptr)
                    {
                        continue;
                    }

                    const auto renderablePos = pRenderable->mWorldPos;
                    const auto radius = pPoint->mRadius;

                    const bool intersection = mg::rayIntersectsSphere(rayOrigin, rayDir, renderablePos, radius);

                    if (intersection)
                    {
                        mSelectedRenderables.insert(pRenderable->id);
                    }
                }
            }
        }
        break;
    case IWindow::Message::MOUSE_MIDDLE_DOWN:
        const auto eventData = mpWindow->getEventData<IWindow::Event::MousePosition>();
        mCursorPos = mCamera.getPos() + mCamera.getFront() * 4.0f;
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

        if (mIsLeftMouseClicked && (!mIsUiClicked) && !(mSelectedRenderables.empty()))
        {
            const float pitchOffset = yOffset * 0.2f;
            const float yawOffset = xOffset * 0.2f;

            if ((mSelectedRenderables.size() > 1) && (mInteractionType == InteractionType::ROTATE))
            {
                mPivotEulerAngles = mPivotEulerAngles + XMVectorSet(pitchOffset, yawOffset, 0, 0);
                break;
            }

            for (const auto selectedRenderableId : mSelectedRenderables)
            {
                const auto pRenderable = mpRenderer->getRenderable(selectedRenderableId);

                switch (mInteractionType)
                {
                case InteractionType::ROTATE:
                    pRenderable->mEulerAngles += XMVectorSet(pitchOffset, yawOffset, 0, 0);
                    break;
                case InteractionType::MOVE:
                    pRenderable->mWorldPos += XMVectorSet(xOffset * 0.1f, yOffset * 0.1f, 0.0f, 0.0f);
                    break;
                case InteractionType::SCALE:
                    const float scaleValue = -yOffset * 0.1f;
                    pRenderable->mScale += XMVectorSet(scaleValue, scaleValue, scaleValue, 1);
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

    ImGui::Combo("##interactionType", reinterpret_cast<int*>(&mInteractionType), interationsNames, IM_ARRAYSIZE(interationsNames));
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

    std::vector<std::string> renderableNames{};
    for (int i = 0; i < mpRenderer->mRenderables.size(); ++i)
    {
        const bool isSelected = std::ranges::find(mSelectedRenderables, mpRenderer->mRenderables.at(i)->id) != mSelectedRenderables.end();

        std::ostringstream renderableName;
        renderableName << std::left << std::setw(renderablesListWidth) << mpRenderer->mRenderables.at(i)->mTag
                       << (isSelected ? "[X]" : "");

        renderableNames.emplace_back(renderableName.str());
    }

    std::vector<const char*> renderableNamesPtrs{};
    for (const auto& name : renderableNames)
    {
        renderableNamesPtrs.push_back(name.c_str());
    }

    int newSelectedItemId = -1;

    ImGui::Text("Renderables:");
    ImGui::ListBox("##objects", &newSelectedItemId, renderableNamesPtrs.data(), static_cast<int>(renderableNamesPtrs.size()));
    mIsUiClicked |= ImGui::IsItemActive();

    bool isNewItemSelected{};
    if (newSelectedItemId != -1)
    {
        int i = 0;
        for (const auto renderableSelectedId : mSelectedRenderables)
        {
            const std::unique_ptr<Renderable>& pRenderable = mpRenderer->mRenderables.at(renderableSelectedId);

            XMVECTOR worldPos = pRenderable->mWorldPos;

            XMMATRIX model = XMMatrixIdentity();

            XMMATRIX localScaleMat = XMMatrixScalingFromVector(pRenderable->mScale);
            XMMATRIX pivotScaleMat = XMMatrixIdentity();

            XMMATRIX pitchRotationMatrix = XMMatrixRotationX(XMVectorGetX(pRenderable->mEulerAngles));
            XMMATRIX yawRotationMatrix = XMMatrixRotationY(XMVectorGetY(pRenderable->mEulerAngles));
            XMMATRIX rollRotationMatrix = XMMatrixRotationZ(XMVectorGetZ(pRenderable->mEulerAngles));

            XMMATRIX localRotationMat = pitchRotationMatrix * yawRotationMatrix * rollRotationMatrix;
            XMMATRIX pivotRotationMat = XMMatrixIdentity();

            if (mSelectedRenderables.size() > 1)
            {
                XMMATRIX pivotPitchRotationMat = XMMatrixRotationX(XMVectorGetX(mPivotEulerAngles));
                XMMATRIX pivotYawRotationMat = XMMatrixRotationY(XMVectorGetY(mPivotEulerAngles));
                XMMATRIX pivotRollRotationMat = XMMatrixRotationZ(XMVectorGetX(mPivotEulerAngles));

                pivotRotationMat = pivotPitchRotationMat * pivotYawRotationMat * pivotRollRotationMat;

                pivotScaleMat = XMMatrixScalingFromVector(pRenderable->mScale);
            }

            XMMATRIX worldTranslation = XMMatrixTranslationFromVector(worldPos);

            XMMATRIX translationToOrigin = XMMatrixTranslationFromVector(-(mPivotPos - worldPos));
            XMMATRIX translationBack = XMMatrixTranslationFromVector((mPivotPos - worldPos));

            model = localScaleMat * localRotationMat *
                    translationToOrigin * pivotRotationMat * translationBack *
                    worldTranslation;

            XMVECTOR scale, rotationQuat, translation;
            ASSERT(XMMatrixDecompose(&scale, &rotationQuat, &translation, model));
            pRenderable->mWorldPos = translation;
            //pRenderable->mScale = scale;

            XMMATRIX rotationMat = localRotationMat * translationToOrigin * pivotRotationMat * translationBack;
            pRenderable->mEulerAngles = mg::getPitchYawRollFromRotationMat(rotationMat);
        }

        mPivotEulerAngles = XMVectorZero();

        const auto newId = mpRenderer->mRenderables.at(newSelectedItemId)->id;
        if (!mSelectedRenderables.contains(newId))
        {
            mSelectedRenderables.insert(newId);
            isNewItemSelected = true;
        }
        else
        {
            mSelectedRenderables.erase(newId);
        }
    }

    if (mSelectedRenderables.size() == 1)
    {
        ImGui::Separator();

        const IRenderable::Id selectedRenderableId = *mSelectedRenderables.begin();

        bool dataChanged{};
        auto pSelectedRenderable = mpRenderer->getRenderable(selectedRenderableId);

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

        float localPos[3];
        float worldPos[3];
        float rotation[3];
        float scale[3];

        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(localPos), pSelectedRenderable->mLocalPos);
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(worldPos), pSelectedRenderable->mWorldPos);
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(rotation), pSelectedRenderable->mEulerAngles);
        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(scale),    pSelectedRenderable->mScale);

        ImGui::Text("Local Position:");
        if (ImGui::InputFloat3("##localPos", localPos))
        {
            pSelectedRenderable->mLocalPos = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(localPos));
            dataChanged = true;
        }
        dataChanged |= ImGui::IsItemActive();

        ImGui::Text("World Position:");
        if (ImGui::InputFloat3("##worldPos", worldPos))
        {
            pSelectedRenderable->mWorldPos = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(worldPos));
            dataChanged = true;
        }
        dataChanged |= ImGui::IsItemActive();

        ImGui::Text("Rotation:");
        if (ImGui::InputFloat3("##rotation", rotation))
        {
            pSelectedRenderable->mEulerAngles = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(rotation));
            dataChanged = true;
        }
        dataChanged |= ImGui::IsItemActive();

        ImGui::Text("Scale:");
        if (ImGui::InputFloat3("##scale", scale))
        {
            pSelectedRenderable->mScale = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(scale));
            dataChanged = true;
        }
        dataChanged |= ImGui::IsItemActive();

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

    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}