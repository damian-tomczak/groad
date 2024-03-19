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
using namespace DirectX;

export class App final : public NonCopyableAndMoveable
{
    static constexpr float aspectRatioFactor = 0.25f;

public:
    App(const ParsedOptions&& options);
    ~App();

    void init();
    void run();

    void renderScene();

private:
    void processInput(IWindow::Message msg, float deltaTime);
    void renderUi();

    XMMATRIX mWorldMatrix = XMMatrixIdentity();

    const ParsedOptions mOptions;
    Camera mCamera{0.0f, 0.5f, -10.0f};
    XMVECTOR mCursorPos{};

    std::shared_ptr<IWindow> mpWindow;
    std::unique_ptr<DX11Renderer> mpRenderer;

    enum class InteractionType
    {
        ROTATE,
        MOVE,
        CURSOR,
    } mInteractionType{};
    inline static const char* interationsNames[] = {"ROTATE", "MOVE", "CURSOR"};

    bool mIsRunning{};
    bool mIsMenuEnabled{};
    bool mIsUiClicked{};
    bool mIsLeftMouseClicked{};
    bool mIsRightMouseClicked{};

    int mSelectedRenderableIdx = -1;
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

        renderScene();
    }
}

void App::renderScene()
{
    mpRenderer->buildGeometryBuffers();

    ID3D11DeviceContext* const pContext = mpRenderer->getContext();

    pContext->ClearRenderTargetView(mpRenderer->getRenderTargetView(),
                                                             reinterpret_cast<const float*>(&Colors::Blue));
    pContext->ClearDepthStencilView(mpRenderer->getDepthStencilView(),
                                                             D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    //pContext->RSSetState(mpRenderer->getWireframeRS());

    const XMMATRIX view = mCamera.getViewMatrix();
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(mCamera.getZoom()),
                                                        mpWindow->getAspectRatio(), 1.0f, 1000.0f);

    ConstantBufferData data
    {
        .model = XMMatrixTranspose(mWorldMatrix),
        .view = XMMatrixTranspose(view),
        .invView = XMMatrixTranspose(XMMatrixInverse(nullptr, view)),
        .proj = XMMatrixTranspose(proj),
        .invProj = XMMatrixTranspose(XMMatrixInverse(nullptr, proj)),
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
        const XMMATRIX translationMat = XMMatrixTranslation(XMVectorGetX(mCursorPos), XMVectorGetY(mCursorPos), XMVectorGetZ(mCursorPos));
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
        XMVECTOR pos = pRenderable->getPosition();

        XMMATRIX translationMatrix = XMMatrixTranslation(XMVectorGetX(pos), XMVectorGetY(pos), XMVectorGetZ(pos));

        data.model = XMMatrixTranspose(translationMatrix);
        data.isSelected = (i == mSelectedRenderableIdx);

        pContext->Map(mpRenderer->getConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
        memcpy(cbData.pData, &data, sizeof(data));
        pContext->Unmap(mpRenderer->getConstantBuffer(), 0);

        UINT vStride = sizeof(XMFLOAT3), offset = 0;
        pContext->IASetVertexBuffers(0, 1, mpRenderer->mVertexBuffers.at(i).GetAddressOf(), &vStride, &offset);
        pContext->IASetIndexBuffer(mpRenderer->mIndexBuffers.at(i).Get(), DXGI_FORMAT_R32_UINT, 0);
        pContext->IASetInputLayout(mpRenderer->getInputLayout());
        pContext->VSSetShader(mpRenderer->mpVS.Get(), nullptr, 0);
        pContext->PSSetShader(mpRenderer->mpPS.Get(), nullptr, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        pContext->DrawIndexed(static_cast<UINT>(pRenderable->getTopology().size()), 0, 0);
    }

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
    //case IWindow::Message::RESIZE:
    //    mpRenderer->onResize();
    //    break;
    case IWindow::Message::KEY_DELETE_DOWN:
        mpRenderer->removeRenderable(mSelectedRenderableIdx);
        mSelectedRenderableIdx = -1;
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
    case IWindow::Message::MOUSE_LEFT_DOWN:
        mIsLeftMouseClicked = true;

        if (mInteractionType == InteractionType::CURSOR)
        {
            mCursorPos = mCamera.getLookingAtVec();
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
    case IWindow::Message::MOUSE_MOVE:
        {
            if (mIsLeftMouseClicked && (!mIsUiClicked))
            {
                const auto mouseEvent = mpWindow->getEvent<IWindow::Event::MousePosition>();

                const float xoffset = -mouseEvent.xoffset * mouseSensitivity;
                const float yoffset = -mouseEvent.yoffset * mouseSensitivity;

                switch (mInteractionType)
                {
                case InteractionType::ROTATE: {
                    static float yaw{};
                    static float pitch{};

                    yaw += xoffset;
                    pitch += yoffset;
                    XMMATRIX rotationY = XMMatrixRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), XMConvertToRadians(yaw));
                    XMMATRIX rotationX = XMMatrixRotationAxis(XMVectorSet(1.f, 0.f, 0.f, 0.f), XMConvertToRadians(pitch));

                    mWorldMatrix = XMMatrixMultiply(mWorldMatrix, rotationY);
                    mWorldMatrix = XMMatrixMultiply(mWorldMatrix, rotationX);
                }
                break;
                case InteractionType::MOVE: {
                    const auto pRenderable = mpRenderer->getRenderable(mSelectedRenderableIdx);
                    const auto pos = pRenderable->getPosition();

                    pRenderable->setPosition(XMVectorAdd(pos, XMVectorSet(xoffset, yoffset, 0.0f, 0.0f)));
                }
                break;
                }
            }
            else if (mIsRightMouseClicked && (!mIsUiClicked))
            {
                const auto mouseEvent = mpWindow->getEvent<IWindow::Event::MousePosition>();
                mCamera.rotateCamera(static_cast<float>(mouseEvent.xoffset), static_cast<float>(-mouseEvent.yoffset));
            }
        }
        break;
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

    ImGui::Text("CAD: ");
    ImGui::Spacing();

    mIsUiClicked = false;

    if (ImGui::Button("Add Torus"))
    {
        mpRenderer->addRenderable(std::move(std::make_unique<Torus>(0.7f, 0.2f, 100, 20)));
    }
    ImGui::SameLine();

    if (ImGui::Button("Add Point"))
    {
        mpRenderer->addRenderable(std::move(std::make_unique<Point>()));
    }

    std::vector<const char*> renderableNames{};
    for (const auto& renderable : mpRenderer->mRenderables)
    {
        if (renderable == nullptr)
        {
            continue;
        }

        renderableNames.emplace_back(renderable->mTag.c_str());
    }

    int newSelectedItem = -1;

    ImGui::Text("Renderables:");
    ImGui::ListBox("##objects", &newSelectedItem, renderableNames.data(), static_cast<int>(renderableNames.size()));

    bool isNewItemSelected = (newSelectedItem != -1) && (mSelectedRenderableIdx != newSelectedItem);
    if (isNewItemSelected)
    {
        mSelectedRenderableIdx = newSelectedItem;
    }

    if (mSelectedRenderableIdx != -1)
    {
        ImGui::Separator();

        bool dataChanged{};
        auto selectedRenderable = mpRenderer->getRenderable(mSelectedRenderableIdx);

        if (auto pTorus = dynamic_cast<Torus*>(selectedRenderable.get()); pTorus != nullptr)
        {
            ImGui::Text("Selected item: %s",selectedRenderable->mTag.c_str());

            static char nameBuffer[256]{};

            if (isNewItemSelected)
            {
                strncpy_s(nameBuffer, selectedRenderable->mTag.c_str(), sizeof(nameBuffer) - 1);
                nameBuffer[sizeof(nameBuffer) - 1] = '\0';
            }

            ImGui::InputText("##tag", nameBuffer, sizeof(nameBuffer));
            dataChanged |= ImGui::IsItemActive();

            ImGui::SameLine();

            if (ImGui::Button("Rename"))
            {
                selectedRenderable->mTag = nameBuffer;
            }

            dataChanged |= ImGui::IsItemActive();

            ImGui::Combo("##interactionType", reinterpret_cast<int*>(&mInteractionType), interationsNames, IM_ARRAYSIZE(interationsNames));

            ImGui::Spacing();

            ImGui::Text("Major Radius:");
            ImGui::SliderFloat("##majorRadius", &pTorus->mMajorRadius, 0.0f, 1.0f);
            dataChanged |= ImGui::IsItemActive();

            ImGui::Text("Minor Radius:");
            ImGui::SliderFloat("##minorRadius", &pTorus->mMinorRadius, 0.0f, 1.0f);
            dataChanged |= ImGui::IsItemActive();

            ImGui::Spacing();

            ImGui::Text("Major Segments:");
            ImGui::SliderInt("##majorSegments", &pTorus->mMajorSegments, 3, 100);
            dataChanged |= ImGui::IsItemActive();

            ImGui::Text("Minor Segments:");
            ImGui::SliderInt("##minorSegments", &pTorus->mMinorSegments, 3, 100);
            dataChanged |= ImGui::IsItemActive();

            if (dataChanged)
            {
                selectedRenderable->generateGeometry();
                selectedRenderable->generateTopology();
                mpRenderer->buildGeometryBuffers();
            }

            mIsUiClicked |= dataChanged;
        }
        else if (auto pPoint = dynamic_cast<Point*>(selectedRenderable.get()); pPoint != nullptr)
        {

        }
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}