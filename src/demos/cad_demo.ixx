module;

#include "utils.h"
#include "imgui.h"
#include "mg.hpp"

#ifdef WIN32
#include <shobjidl.h>
#include <windows.h>
#endif

export module cad_demo;
import core;
import dx11renderer;

export class CADDemo : public IDemo
{
    static constexpr float renderablesListHeight = 0.3f;

public:
    CADDemo(Context& ctx, IRenderer* pRenderer, std::shared_ptr<IWindow> pWindow);
    ~CADDemo();

    void init() override;
    void update(float dt) override;
    void draw() override;
    void processInput(IWindow::Message msg, float dt) override;
    void renderUi() override;

private:
    std::optional<BezierSurfaceCreator> mBezierPatchCreator;

    void drawBernsteins(const std::vector<XMFLOAT3>& bernsteinPoints);

    int mSelectedBernsteinPointIndex = -1;

    enum Message
    {
        EMPTY,
        UNABLE_TO_DELETE,
        MESSAGE_COUNT
    };

    static constexpr auto messageSize = sizeof(Message) * 8;

    std::bitset<messageSize> mMessage;

    inline static std::array<std::function<void(std::bitset<messageSize>& message)>, messageSize> messagePopups{
        [](std::bitset<messageSize>& message){}, // EMPTY
        [](std::bitset<messageSize>& message)
        {
            ImGui::OpenPopup("UNABLE_TO_DELETE");
            if (ImGui::BeginPopup("UNABLE_TO_DELETE"))
            {
                float textWidth = ImGui::CalcTextSize("Renderable isn't deletable!").x;
                float windowWidth = ImGui::GetWindowSize().x;
                ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);

                ImGui::Text("Renderable isn't deletable!");

                float buttonWidth = ImGui::CalcTextSize("Ok").x + ImGui::GetStyle().FramePadding.x * 2;
                ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);

                if (ImGui::Button("Ok"))
                {
                    message.reset();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        },
    };

    void saveScene();
    void loadScene();

    bool mIsFileDialogOpen{};

    void applyRenderableRotations()
    {
        for (const auto renderableSelectedId : mCtx.renderableSelection.renderableIds)
        {
            IRenderable* const pRenderable = mpRenderer->getRenderable(renderableSelectedId);

            XMMATRIX model = XMMatrixIdentity();

            XMMATRIX localScaleMat = XMMatrixScalingFromVector(pRenderable->mScale);
            XMMATRIX centroidScaleMat = XMMatrixIdentity();

            XMMATRIX pitchRotationMatrix = XMMatrixRotationX(pRenderable->mPitch);
            XMMATRIX yawRotationMatrix = XMMatrixRotationY(pRenderable->mYaw);
            XMMATRIX rollRotationMatrix = XMMatrixRotationZ(pRenderable->mRoll);

            XMMATRIX localRotationMat = pitchRotationMatrix * yawRotationMatrix * rollRotationMatrix;
            XMMATRIX centroidRotationMat = XMMatrixIdentity();

            XMMATRIX translationToOrigin = XMMatrixIdentity();
            XMMATRIX translationBack = XMMatrixIdentity();

            if (mCtx.renderableSelection.renderableIds.contains(pRenderable->getId()))
            {
                XMMATRIX centroidPitchRotationMat = XMMatrixRotationX(mCtx.centroidPitch);
                XMMATRIX centroidYawRotationMat = XMMatrixRotationY(mCtx.centroidYaw);
                XMMATRIX centroidRollRotationMat = XMMatrixRotationZ(mCtx.centroidRoll);

                centroidRotationMat = centroidPitchRotationMat * centroidYawRotationMat * centroidRollRotationMat;

                centroidScaleMat = XMMatrixScaling(mCtx.centroidScale, mCtx.centroidScale, mCtx.centroidScale);

                translationToOrigin = XMMatrixTranslationFromVector(-(mCtx.centroidPos - pRenderable->mWorldPos));
                translationBack = XMMatrixTranslationFromVector((mCtx.centroidPos - pRenderable->mWorldPos));
            }

            XMMATRIX worldTranslation = XMMatrixTranslationFromVector(pRenderable->mWorldPos);

            model = localScaleMat * localRotationMat *
                translationToOrigin * centroidScaleMat * centroidRotationMat * translationBack *
                worldTranslation;

            pRenderable->mWorldPos = model.r[3];

            const XMMATRIX scaleMat = localScaleMat * centroidScaleMat;
            pRenderable->mScale = XMVECTOR{ XMVectorGetX(scaleMat.r[0]), XMVectorGetY(scaleMat.r[1]), XMVectorGetZ(scaleMat.r[2]), 1.f };

            const XMMATRIX rotationMat =
                localRotationMat * translationToOrigin * centroidRotationMat * translationBack;
            const auto [pitch, yaw, roll] = mg::getPitchYawRollFromRotationMat(rotationMat);

            pRenderable->mPitch = pitch;
            pRenderable->mYaw = yaw;
            pRenderable->mRoll = roll;
        }

        mCtx.centroidPitch = 0;
        mCtx.centroidYaw = 0;
        mCtx.centroidRoll = 0;

        mCtx.centroidScale = 1;
    }

    // TODO: refactor it
    bool mIsStereoscopy{};
    ComPtr<ID3D11SamplerState> mpBlendSamplerState;
    struct RenderTarget
    {
        RenderTarget(IRenderer* pRenderer, unsigned width, unsigned height)
        {
            auto pDX11Renderer = static_cast<DX11Renderer*>(pRenderer);

            if (width == 0 || height == 0)
            {
                renderTargetView = nullptr;
                depthStencilView = nullptr;

                return;
            }

            D3D11_TEXTURE2D_DESC desc
            {
                .Width = width,
                .Height = height,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
                .SampleDesc = 
                {
                    .Count = 1,
                    .Quality = 0,
                },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
            };

            HR(pDX11Renderer->getDevice()->CreateTexture2D(&desc, nullptr, targetTexture.GetAddressOf()));

            HR(pDX11Renderer->getDevice()->CreateRenderTargetView(targetTexture.Get(), nullptr, renderTargetView.GetAddressOf()));

            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_D32_FLOAT;
            desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

            ComPtr<ID3D11Texture2D> depthStencilTexture;
            HR(pDX11Renderer->getDevice()->CreateTexture2D(&desc, nullptr, depthStencilTexture.GetAddressOf()));

            ComPtr<ID3D11DepthStencilView> result;
            HR(pDX11Renderer->getDevice()->CreateDepthStencilView(depthStencilTexture.Get(), nullptr, depthStencilView.GetAddressOf()));

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = 1;

            HR(pDX11Renderer->getDevice()->CreateShaderResourceView(targetTexture.Get(), &srvDesc, shaderResourceView.GetAddressOf()));
        }

        ComPtr<ID3D11Texture2D> targetTexture;
        ComPtr<ID3D11RenderTargetView> renderTargetView;
        ComPtr<ID3D11ShaderResourceView> shaderResourceView;
        ComPtr<ID3D11DepthStencilView> depthStencilView;

        UINT width, height;
    };
    enum class Eye : short
    {
        INVALID = -1,
        LEFT = 0,
        RIGHT = 1,
    };
    Eye mEyeForCurrentRendering = Eye::INVALID;
    unsigned int mPreviousWidth{}, mPreviousHeight{};
    std::unordered_map<Eye, std::unique_ptr<RenderTarget>> mEyeRenderTargets;
    std::unordered_map<Eye, XMMATRIX> mProjectionMatricesForEyes;
    struct StereoscopySettings
    {
        float eyeDistance = 0.05f;
        float focusPlane = 10.0f;
    } mStereoscopySettings;
    bool mIsOpenStereoscopyPopup = false;
};

class GridSurface : public ISurface
{
public:
    GridSurface(IRenderer* pRenderer) : ISurface{pRenderer}
    {
    }

    void init() override{};
    void draw() override
    {
        auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

        pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().gridVS.first.Get(), nullptr, 0);
        pDX11Renderer->getContext()->HSSetShader(nullptr, nullptr, 0);
        pDX11Renderer->getContext()->DSSetShader(nullptr, nullptr, 0);
        pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
        pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().gridPS.first.Get(), nullptr, 0);
        pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pDX11Renderer->getContext()->Draw(6, 0);
    }

    void update(float dt) override
    {
    }
};

module :private;

CADDemo::CADDemo(Context& ctx, IRenderer* pRenderer, std::shared_ptr<IWindow> pWindow)
    : IDemo{ctx, "CADDemo", pRenderer, pWindow, std::make_unique<GridSurface>(pRenderer)}
{
    reloadCounters();

    mpMenuBarCallback = [this]() {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load Scene"))
            {
                loadScene();
            }

            if (ImGui::MenuItem("Save Scene"))
            {
                saveScene();
            }

            ImGui::EndMenu();
        }

        // TODO: demo's settings
        ImGui::MenuItem(mIsStereoscopy ? "Stereoscopy ON" : "Stereoscopy OFF", nullptr, &mIsStereoscopy);

        if (mIsStereoscopy && (mpBlendSamplerState == nullptr))
        {
            auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

            D3D11_SAMPLER_DESC samplerDesc{};

            samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

            HR(pDX11Renderer->getDevice()->CreateSamplerState(&samplerDesc, mpBlendSamplerState.GetAddressOf()));
        }

        ImGui::MenuItem("Stereoscopy settings", nullptr, &mIsOpenStereoscopyPopup);
    };
}

CADDemo::~CADDemo()
{
}

void CADDemo::init()
{
    IDemo::init();

    //std::vector<Id> deBoorIds;

    //XMVECTOR pos{ -2.f, 3.f, 0 };
    //auto pPoint = std::make_unique<Point>(pos);
    //pPoint->regenerateData();
    //deBoorIds.push_back(pPoint->getId());
    //mpRenderer->addRenderable(std::move(pPoint));

    //pos = XMVECTOR{ -1.f, 3.f, 0 };
    //pPoint = std::make_unique<Point>(pos);
    //pPoint->regenerateData();
    //deBoorIds.push_back(pPoint->getId());
    //mpRenderer->addRenderable(std::move(pPoint));

    //pos = XMVECTOR{ 0.0f, 3.0f, 0.0f };
    //pPoint = std::make_unique<Point>(pos);
    //pPoint->regenerateData();
    //deBoorIds.push_back(pPoint->getId());
    //mpRenderer->addRenderable(std::move(pPoint));

    //pos = XMVECTOR{ 1.0f, 3.0f, 0.0f };
    //pPoint = std::make_unique<Point>(pos);
    //pPoint->regenerateData();
    //deBoorIds.push_back(pPoint->getId());
    //mpRenderer->addRenderable(std::move(pPoint));

    //pos = XMVECTOR{ -2.0f, 1.0f, 0.0f };
    //pPoint = std::make_unique<Point>(pos);
    //pPoint->regenerateData();
    //deBoorIds.push_back(pPoint->getId());
    //mpRenderer->addRenderable(std::move(pPoint));

    //pos = XMVECTOR{ -1.0f, 1.0f, 0.0f };
    //pPoint = std::make_unique<Point>(pos);
    //pPoint->regenerateData();
    //deBoorIds.push_back(pPoint->getId());
    //mpRenderer->addRenderable(std::move(pPoint));

    //pos = XMVECTOR{ 0.0f, 1.0f, 0.0f };
    //pPoint = std::make_unique<Point>(pos);
    //pPoint->regenerateData();
    //deBoorIds.push_back(pPoint->getId());
    //mpRenderer->addRenderable(std::move(pPoint));

    //pos = XMVECTOR{ 1.0f, 1.0f, 0.0f };
    //pPoint = std::make_unique<Point>(pos);
    //pPoint->regenerateData();
    //deBoorIds.push_back(pPoint->getId());
    //mpRenderer->addRenderable(std::move(pPoint));
}

void CADDemo::draw()
{
    //IDemo::draw();

    auto drawScene = [this]() {
        auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

        mpSurface->draw();

        std::vector<Id> pointIdsPartOfSelected;
        for (const IControlPointBased* pControlPointBased : mCtx.renderableSelection.getRenderables<IControlPointBased>(mpRenderer))
        {
            pointIdsPartOfSelected.insert(
                pointIdsPartOfSelected.end(),
                pControlPointBased->mControlPointIds.begin(),
                pControlPointBased->mControlPointIds.end()
            );
        }

        for (const auto [renderableIndex, pRenderable] : std::views::enumerate(pDX11Renderer->getRenderables()))
        {
            if (pRenderable == nullptr)
            {
                continue;
            }

            pDX11Renderer->mGlobalCB.color = pRenderable->mColor;
            pDX11Renderer->mGlobalCB.tesFactor = pRenderable->mTesFactor;

            XMMATRIX model = XMMatrixIdentity();

            XMMATRIX localScaleMat = XMMatrixScalingFromVector(pRenderable->mScale);
            XMMATRIX centroidScaleMat = XMMatrixIdentity();

            XMMATRIX pitchRotationMatrix = XMMatrixRotationX(pRenderable->mPitch);
            XMMATRIX yawRotationMatrix = XMMatrixRotationY(pRenderable->mYaw);
            XMMATRIX rollRotationMatrix = XMMatrixRotationZ(pRenderable->mRoll);

            XMMATRIX localRotationMat = pitchRotationMatrix * yawRotationMatrix * rollRotationMatrix;
            XMMATRIX centroidRotationMat = XMMatrixIdentity();

            XMMATRIX translationToOrigin = XMMatrixIdentity();
            XMMATRIX translationBack = XMMatrixIdentity();

            if (mCtx.renderableSelection.renderableIds.contains(pRenderable->getId()))
            {
                XMMATRIX centroidPitchRotationMat = XMMatrixRotationX(mCtx.centroidPitch);
                XMMATRIX centroidYawRotationMat = XMMatrixRotationY(mCtx.centroidYaw);
                XMMATRIX centroidRollRotationMat = XMMatrixRotationZ(mCtx.centroidRoll);

                centroidRotationMat = centroidPitchRotationMat * centroidYawRotationMat * centroidRollRotationMat;

                centroidScaleMat = XMMatrixScaling(mCtx.centroidScale, mCtx.centroidScale, mCtx.centroidScale);

                translationToOrigin = XMMatrixTranslationFromVector(-(mCtx.centroidPos - pRenderable->mWorldPos));
                translationBack = XMMatrixTranslationFromVector((mCtx.centroidPos - pRenderable->mWorldPos));
            }

            XMMATRIX worldTranslation = XMMatrixTranslationFromVector(pRenderable->mWorldPos);

            model = localScaleMat * localRotationMat *
                translationToOrigin * centroidScaleMat * centroidRotationMat * translationBack *
                worldTranslation;

            pDX11Renderer->mGlobalCB.modelMtx = model;

            const bool isPointSelected = std::ranges::find(pointIdsPartOfSelected, pRenderable->getId()) != pointIdsPartOfSelected.end();

            if (mCtx.renderableSelection.renderableIds.contains(pRenderable->getId()) || isPointSelected)
            {
                pDX11Renderer->mGlobalCB.color = defaultSelectionColor;
            }

            pDX11Renderer->updateCB(pDX11Renderer->mGlobalCB);

            ID3D11Buffer* const* const pVertexBuffer = pDX11Renderer->mVertexBuffers.at(renderableIndex).GetAddressOf();
            //ASSERT(*pVertexBuffer != nullptr);
            pDX11Renderer->getContext()->IASetVertexBuffers(0, 1, pVertexBuffer, &pRenderable->getStride(), &pRenderable->getOffset());

            const ComPtr<ID3D11Buffer> indexBuffer = pDX11Renderer->mIndexBuffers.at(renderableIndex);
            if (indexBuffer != nullptr)
            {
                pDX11Renderer->getContext()->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
            }

            pRenderable->draw(mpRenderer, renderableIndex);
        }

        if (mCtx.renderableSelection.renderableIds.size() == 1)
        {
            Id selectedRenderableId = *mCtx.renderableSelection.renderableIds.begin();
            IRenderable* pRenderable = pDX11Renderer->getRenderable(selectedRenderableId);
            if (auto pBezierC2 = dynamic_cast<BezierCurveC2*>(pRenderable); pBezierC2 != nullptr)
            {
                drawBernsteins(pBezierC2->getBernsteinPositions());
            }
        }
        else if (mCtx.renderableSelection.renderableIds.size() > 1)
        {
            XMVECTOR sum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
            for (const auto& selectedRenderableId : mCtx.renderableSelection.renderableIds)
            {
                const auto pRenderable = pDX11Renderer->getRenderable(selectedRenderableId);
                sum = XMVectorAdd(sum, pRenderable->getGlobalPos());
            }

            float numPositions = static_cast<float>(mCtx.renderableSelection.renderableIds.size());
            mCtx.centroidPos = XMVectorScale(sum, 1.0f / numPositions);

            float radius = Point::defaultRadius;
            std::vector<Point*> renderablePoints = mCtx.renderableSelection.getRenderables<Point>(mpRenderer);
            if (renderablePoints.size() == mCtx.renderableSelection.renderableIds.size())
            {
                radius = 0;
                for (const Point* pPoint : renderablePoints)
                {
                    radius += pPoint->mRadius;
                }
            }

            Point::drawPrimitive(mpRenderer, mCtx.centroidPos, radius / renderablePoints.size());
        }

        {
            pDX11Renderer->mGlobalCB.modelMtx = XMMatrixTranslationFromVector(mCtx.cursorPos);

            pDX11Renderer->updateCB(pDX11Renderer->mGlobalCB);

            pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().cursorVS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->HSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->DSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->GSSetShader(pDX11Renderer->getShaders().cursorGS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().cursorPS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

            pDX11Renderer->getContext()->Draw(6, 0);
        }
    };

    if (!mIsStereoscopy)
    {
        drawScene();
    }
    else
    {
        auto updateProjectionMatrices = [this](float aspectRatio, float fov, float nearPlane, float farPlane) -> void {
            float focusPlane = mStereoscopySettings.focusPlane;
            float eyeDistance = mStereoscopySettings.eyeDistance;
            float halfEyeDistance = eyeDistance * 0.5f;

            float top = focusPlane * tanf(DirectX::XMConvertToRadians(fov * 0.5f));
            float bottom = focusPlane * tanf(-DirectX::XMConvertToRadians(fov * 0.5f));

            float width = (top - bottom) * aspectRatio;

            float L = -0.5f * width;
            float R = 0.5f * width;

            float leftEyeL = (L + halfEyeDistance) * nearPlane / focusPlane;
            float leftEyeR = (R + halfEyeDistance) * nearPlane / focusPlane;

            float rightEyeL = (L - halfEyeDistance) * nearPlane / focusPlane;
            float rightEyeR = (R - halfEyeDistance) * nearPlane / focusPlane;

            top = top * nearPlane / focusPlane;
            bottom = bottom * nearPlane / focusPlane;

            mProjectionMatricesForEyes[Eye::LEFT] = XMMatrixTranslation(halfEyeDistance, 0.0f, 0.0f)
                * XMMatrixPerspectiveOffCenterLH(leftEyeL, leftEyeR, bottom, top, nearPlane, farPlane);

            mProjectionMatricesForEyes[Eye::RIGHT] = XMMatrixTranslation(-halfEyeDistance, 0.0f, 0.0f)
                * XMMatrixPerspectiveOffCenterLH(rightEyeL, rightEyeR, bottom, top, nearPlane, farPlane);
        };

        auto renderSceneForEye = [this, drawScene](Eye eye) {
            auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

            mEyeForCurrentRendering = eye;

            auto& renderTarget = mEyeRenderTargets[eye];

            ASSERT(renderTarget != nullptr);

            const float clearColor[]{ 0.0f, 0.0f, 0.0f, 0.0f };
            pDX11Renderer->getContext()->ClearRenderTargetView(renderTarget->renderTargetView.Get(), clearColor);
            pDX11Renderer->getContext()->ClearDepthStencilView(renderTarget->depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
            pDX11Renderer->getContext()->OMSetRenderTargets(1, renderTarget->renderTargetView.GetAddressOf(), renderTarget->depthStencilView.Get());

            pDX11Renderer->mGlobalCB.proj = mProjectionMatricesForEyes[eye];

            drawScene();
        };

        auto updateEyeRenderTargets = [this](unsigned int width, unsigned int height) {
            if ((width == mPreviousWidth) && (height == mPreviousHeight))
            {
                return;
            }

            mEyeRenderTargets[Eye::LEFT] = std::make_unique<RenderTarget>(mpRenderer, width, height);
            mEyeRenderTargets[Eye::RIGHT] = std::make_unique<RenderTarget>(mpRenderer, width, height);

            mPreviousWidth = width;
            mPreviousHeight = height;
        };

        auto blendEyesToSingle = [this]() {
            auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

            pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            auto& leftEye = mEyeRenderTargets[Eye::LEFT];
            auto& rightEye = mEyeRenderTargets[Eye::RIGHT];

            if (!leftEye || !rightEye)
            {
                return;
            }

            pDX11Renderer->getContext()->OMSetRenderTargets(1, pDX11Renderer->getAddressOfRenderTargetView(), pDX11Renderer->getDepthStencilView());
            pDX11Renderer->getContext()->RSSetViewports(1, pDX11Renderer->getAddressOfScreenViewport());
            pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().blendVS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->HSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->DSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().blendPS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->RSSetState(nullptr);

            ID3D11ShaderResourceView* eyeSRV[] = { leftEye->shaderResourceView.Get(), rightEye->shaderResourceView.Get() };

            pDX11Renderer->getContext()->PSSetSamplers(0, 1, mpBlendSamplerState.GetAddressOf());
            pDX11Renderer->getContext()->PSSetShaderResources(0, 2, eyeSRV);

            pDX11Renderer->getContext()->Draw(6, 0);
        };

        auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

        updateProjectionMatrices(mpWindow->getAspectRatio(), mCamera.getZoom(), 0.01f, 100.0f);

        updateEyeRenderTargets(mpWindow->getWidth(), mpWindow->getHeight());

        renderSceneForEye(Eye::LEFT);
        renderSceneForEye(Eye::RIGHT);

        blendEyesToSingle();
    }
}

void CADDemo::processInput(IWindow::Message msg, float dt)
{
    auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

    switch (msg)
    {
    case IWindow::Message::MOUSE_LEFT_DOWN: {
        if (mCtx.interactionType == InteractionType::SELECT)
        {
            const auto eventData = mpWindow->getEventData<IWindow::Event::MousePosition>();

            float mouseX = static_cast<float>(eventData.x);
            float mouseY = static_cast<float>(eventData.y);
            float x = (2.0f * mouseX) / mpWindow->getWidth() - 1.0f;
            float y = 1.0f - (2.0f * mouseY) / mpWindow->getHeight();
            float z = 1.0f;

            XMVECTOR rayNDC = XMVectorSet(x, y, z, 1.0f);

            XMMATRIX invProjMtx = XMMatrixInverse(nullptr, mCamera.mProjMtx);
            XMMATRIX invViewMtx = XMMatrixInverse(nullptr, mCamera.mViewMtx);

            XMVECTOR rayClip = XMVectorSet(x, y, -1.0f, 1.0f);
            XMVECTOR rayView = XMVector4Transform(rayClip, invProjMtx);

            rayView = XMVectorSetW(rayView, 0.0f);

            XMVECTOR rayWorld = XMVector4Transform(rayView, invViewMtx);
            rayWorld = XMVector3Normalize(rayWorld);

            XMVECTOR rayOrigin = mCamera.getPos();
            XMVECTOR rayDir = rayWorld;

            for (const auto& pRenderable : mpRenderer->getRenderables())
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
                    mCtx.renderableSelection.renderableIds.insert(pRenderable->getId());
                }
            }
        }
    }
    break;
    case IWindow::Message::MOUSE_MOVE: {
        const auto eventData = mpWindow->getEventData<IWindow::Event::MousePosition>();

        if ((mCtx.lastXMousePosition == -1) && (mCtx.lastYMousePosition == -1))
        {
            mCtx.lastXMousePosition = eventData.x;
            mCtx.lastYMousePosition = eventData.y;
        }

        const float xOffset = (eventData.x - mCtx.lastXMousePosition) * 10 * mouseSensitivity;
        const float yOffset = (eventData.y - mCtx.lastYMousePosition) * 10 * mouseSensitivity;

        mCtx.lastXMousePosition = eventData.x;
        mCtx.lastYMousePosition = eventData.y;

        if (mCtx.isLeftMouseClicked && (!mCtx.isUiClicked) && (!mCtx.renderableSelection.renderableIds.empty()))
        {
            const float pitchOffset = yOffset * 0.2f;
            const float yawOffset = xOffset * 0.2f;

            if ((mCtx.renderableSelection.renderableIds.size() > 1) && (mCtx.interactionType == InteractionType::ROTATE))
            {
                mCtx.centroidPitch += pitchOffset;
                mCtx.centroidYaw += yawOffset;
                break;
            }

            if ((mCtx.renderableSelection.renderableIds.size() > 1) && (mCtx.interactionType == InteractionType::SCALE))
            {
                mCtx.centroidScale += -yOffset * 0.1f;
                break;
            }

            const XMVECTOR offsetVec = XMVectorSet(xOffset * 0.1f, yOffset * 0.1f, 0.0f, 0.0f);

            for (const Id selectedRenderableId : mCtx.renderableSelection.renderableIds)
            {
                IRenderable* const pSelectedRenderable = mpRenderer->getRenderable(selectedRenderableId);

                if (auto pBezierC2 = dynamic_cast<BezierCurveC2*>(pSelectedRenderable); (pBezierC2 != nullptr) && (mSelectedBernsteinPointIndex != -1))
                {
                    int idx = 0;
                    for (XMFLOAT3& bernsteinPos : pBezierC2->getBernsteinPositions())
                    {
                        if (idx == mSelectedBernsteinPointIndex)
                        {
                            XMVECTOR va = XMLoadFloat3(&bernsteinPos);
                            const XMVECTOR vc = va + offsetVec;

                            XMStoreFloat3(&bernsteinPos, vc);
                            break;
                        }
                        idx++;
                    }

                    pBezierC2->regenerateData();
                    pBezierC2->updateBernstein(mSelectedBernsteinPointIndex, offsetVec);
                }
                else
                {
                    switch (mCtx.interactionType)
                    {
                    case InteractionType::ROTATE:
                        pSelectedRenderable->mPitch += pitchOffset;
                        pSelectedRenderable->mYaw += yawOffset;
                        if (auto pBezierSurface = dynamic_cast<IBezierSurface*>(pSelectedRenderable))
                        {
                            pBezierSurface->rotateControlPoints(pitchOffset, yawOffset, 0.f, 1.f);
                        }
                        break;
                    case InteractionType::MOVE:
                    {
                        if (auto pSelectedPoint = dynamic_cast<Point*>(pSelectedRenderable); pSelectedPoint != nullptr)
                        {
                            pSelectedPoint->mWorldPos += offsetVec;

                            for (const auto& pRenderable : mpRenderer->getRenderables())
                            {
                                if (auto pControlPointBased = dynamic_cast<IControlPointBased*>(pRenderable.get());
                                    pControlPointBased != nullptr)
                                {
                                    pRenderable->regenerateData();
                                }
                            }
                        }
                        else if (auto pSelectedBezierC0 = dynamic_cast<BezierCurveC0*>(pSelectedRenderable); pSelectedBezierC0 != nullptr)
                        {
                            // Selected bezier C0
                            // TODO: update only beziers containing modified control points
                            // TODO: refactor for ControlPointBased
                            for (const Id controlPointRenderableId : pSelectedBezierC0->mControlPointIds)
                            {
                                IRenderable* const pControlPointRenderable =
                                    mpRenderer->getRenderable(controlPointRenderableId);

                                pControlPointRenderable->mWorldPos += offsetVec;
                            }

                            for (const auto& pRenderable : mpRenderer->getRenderables())
                            {
                                auto pRenderableBezier = dynamic_cast<IBezierCurve*>(pRenderable.get());

                                if (pRenderableBezier != nullptr)
                                {
                                    pRenderableBezier->regenerateData();
                                }
                            }
                        }
                        else if (auto pSelectedBezierC2 = dynamic_cast<BezierCurveC2*>(pSelectedRenderable); pSelectedBezierC2 != nullptr)
                        {
                            // Selected bezier C2
                            // TODO: update only beziers containing modified control points
                            for (const Id controlPointRenderableId : pSelectedBezierC2->mControlPointIds)
                            {
                                IRenderable* const pControlPointRenderable =
                                    mpRenderer->getRenderable(controlPointRenderableId);

                                pControlPointRenderable->mWorldPos += offsetVec;
                            }

                            for (const auto& pRenderable : mpRenderer->getRenderables())
                            {
                                auto pRenderableBezier = dynamic_cast<IBezierCurve*>(pRenderable.get());

                                if (pRenderableBezier != nullptr)
                                {
                                    pRenderableBezier->regenerateData();
                                }
                            }
                        }
                        else if (auto pSelectedInterpolatedBezierC2 = dynamic_cast<InterpolatedBezierCurveC2*>(pSelectedRenderable); pSelectedInterpolatedBezierC2 != nullptr)
                        {
                            // Selected interpolated bezier C2
                            // TODO: update only beziers containing modified control points
                            for (const Id controlPointRenderableId : pSelectedInterpolatedBezierC2->mControlPointIds)
                            {
                                IRenderable* const pControlPointRenderable =
                                    mpRenderer->getRenderable(controlPointRenderableId);

                                pControlPointRenderable->mWorldPos += offsetVec;
                            }

                            for (const auto& pRenderable : mpRenderer->getRenderables())
                            {
                                auto pRenderableBezier = dynamic_cast<IBezierCurve*>(pRenderable.get());

                                if (pRenderableBezier != nullptr)
                                {
                                    pRenderableBezier->regenerateData();
                                }
                            }
                        }
                        else if (auto pSelectedBezierSurfaceC0 = dynamic_cast<BezierSurfaceC0*>(pSelectedRenderable); pSelectedBezierSurfaceC0 != nullptr)
                        {
                            for (const Id controlPointId : pSelectedBezierSurfaceC0->mControlPointIds)
                            {
                                IRenderable* const pControlPointRenderable =
                                    mpRenderer->getRenderable(controlPointId);

                                pControlPointRenderable->mWorldPos += offsetVec;
                            }

                            pSelectedBezierSurfaceC0->regenerateData();
                        }
                        else if (auto pSelectedBezierSurfaceC2 = dynamic_cast<BezierSurfaceC2*>(pSelectedRenderable); pSelectedBezierSurfaceC2 != nullptr)
                        {
                            for (const Id controlPointId : pSelectedBezierSurfaceC2->mControlPointIds)
                            {
                                IRenderable* const pControlPointRenderable = mpRenderer->getRenderable(controlPointId);

                                pControlPointRenderable->mWorldPos += offsetVec;
                            }

                            pSelectedBezierSurfaceC2->regenerateData();
                        }
                        else
                        {
                            pSelectedRenderable->mWorldPos += offsetVec;
                        }
                    }
                    break;
                    case InteractionType::SCALE:
                        const float scaleVal = -yOffset * 0.1f;
                        pSelectedRenderable->mScale += XMVECTOR{scaleVal, scaleVal, scaleVal, 1.f};
                        if (auto pBezierSurface = dynamic_cast<IBezierSurface*>(pSelectedRenderable))
                        {
                            if (scaleVal != 0.f && scaleVal != -0.f)
                            {
                                pBezierSurface->rotateControlPoints(0.f, 0.f, 0.f, scaleVal);
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
    break;
    case IWindow::Message::KEY_DELETE_DOWN:
        for (auto it = mCtx.renderableSelection.renderableIds.begin(); it != mCtx.renderableSelection.renderableIds.end();)
        {
            if (!mpRenderer->getRenderable(*it)->isDeletable())
            {
                mMessage.set(Message::UNABLE_TO_DELETE);
                ++it;
                continue;
            }

            pDX11Renderer->removeRenderable(*it);
            mCtx.renderableSelection.renderableIds.erase(*it);
            it = mCtx.renderableSelection.renderableIds.begin();
        }

        // TODO: perf bottleneck
        for (auto& pRenderable : pDX11Renderer->getRenderables())
        {
            if (pRenderable != nullptr)
            {
                pRenderable->regenerateData();
            }
        }
        break;
    }
}

void CADDemo::renderUi()
{
    auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

    mCtx.isMenuEnabled = false;
    mCtx.isUiClicked = false;

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    for (int i = 0; i < MESSAGE_COUNT; ++i)
    {
        if (mMessage.test(i))
        {
            messagePopups[i](mMessage);
        }
    }

    const float menuWidth = ImGui::GetIO().DisplaySize.x * 0.2f;

    ImGui::SetNextWindowPos(ImVec2{ImGui::GetIO().DisplaySize.x - menuWidth, mCtx.menuBarHeight});
    ImGui::SetNextWindowSize(ImVec2{menuWidth, ImGui::GetIO().DisplaySize.y - mCtx.menuBarHeight});

    if (ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        mCtx.isMenuEnabled = true;

        if (ImGui::IsWindowHovered())
        {
            mCtx.isMenuHovered = true;
        }
        else
        {
            mCtx.isMenuHovered = false;
        }

        bool isBernsteinEdited = mSelectedBernsteinPointIndex != -1;

        if (isBernsteinEdited)
        {
            ImGui::BeginDisabled();
        }

        ImGui::Text("CAD: ");
        ImGui::Spacing();

        ImGui::Combo("##interactionType", reinterpret_cast<int*>(&mCtx.interactionType), interationsNames,
                     IM_ARRAYSIZE(interationsNames));
        mCtx.isUiClicked |= ImGui::IsItemActive();

        ImGui::Spacing();

        if (ImGui::Button("Add Torus"))
        {
            auto pTorus = std::make_unique<Torus>(mCtx.cursorPos);
            pTorus->regenerateData();
            mpRenderer->addRenderable(std::move(pTorus));
        }
        ImGui::SameLine();

        if (ImGui::Button("Add Point"))
        {
            IBezierCurve* pBezier = nullptr;
            if (mCtx.renderableSelection.renderableIds.size() == 1)
            {
                const Id selectedRenderableId = *mCtx.renderableSelection.renderableIds.begin();
                IRenderable* const pRenderable = mpRenderer->getRenderable(selectedRenderableId);
                pBezier = dynamic_cast<IBezierCurve*>(pRenderable);
            }

            auto pPoint = std::make_unique<Point>(mCtx.cursorPos);
            pPoint->regenerateData();
            const Id pointId = pPoint->getId();
            mpRenderer->addRenderable(std::move(pPoint));

            if ((pBezier != nullptr) && (pBezier->getId() != invalidId))
            {
                pBezier->insertDeBoorPointId(pointId);
                pBezier->regenerateData();
            }
        }

        if (ImGui::Button("Create surface"))
        {
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::OpenPopup("Bezier Patch Creator");
        }

        if (ImGui::BeginPopup("Bezier Patch Creator"))
        {
            if (mBezierPatchCreator == std::nullopt)
            {
                mBezierPatchCreator = std::make_optional<BezierSurfaceCreator>();
            }

            if (ImGui::RadioButton("Flat", !mBezierPatchCreator->isWrapped))
            {
                mBezierPatchCreator->isWrapped = false;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Wrapped", mBezierPatchCreator->isWrapped))
            {
                mBezierPatchCreator->isWrapped = true;
            }

            ImGui::Checkbox("C2", &mBezierPatchCreator->isC2);

            ImGui::InputInt("U patches", reinterpret_cast<int*>(&mBezierPatchCreator->u));
            if ((!mBezierPatchCreator->isWrapped) && (mBezierPatchCreator->u < 1))
            {
                mBezierPatchCreator->u = 1;
            }
            else if (mBezierPatchCreator->isWrapped && mBezierPatchCreator->isC2 && (mBezierPatchCreator->u < 3))
            {
                mBezierPatchCreator->u = 3;
            }
            else if (mBezierPatchCreator->isWrapped && (mBezierPatchCreator->u < 2))
            {
                mBezierPatchCreator->u = 2;
            }

            ImGui::InputInt("V patches", reinterpret_cast<int*>(&mBezierPatchCreator->v));
            if ((!mBezierPatchCreator->isWrapped) && (mBezierPatchCreator->v < 1))
            {
                mBezierPatchCreator->v = 1;
            }
            else if (mBezierPatchCreator->isWrapped && mBezierPatchCreator->isC2 && (mBezierPatchCreator->v < 3))
            {
                mBezierPatchCreator->v = 3;
            }
            else if (mBezierPatchCreator->isWrapped && (mBezierPatchCreator->v < 2))
            {
                mBezierPatchCreator->v = 2;
            }

            ImGui::InputFloat("patch Size U", &mBezierPatchCreator->patchSizeU);
            ImGui::InputFloat("patch Size V", &mBezierPatchCreator->patchSizeV);

            if (ImGui::Button("Create"))
            {
                std::unique_ptr<IRenderable> pRenderable;
                if (!mBezierPatchCreator->isC2)
                {
                    pRenderable =
                        std::make_unique<BezierSurfaceC0>(mCtx.cursorPos, std::move(*mBezierPatchCreator), mpRenderer);
                }
                else
                {
                    pRenderable =
                        std::make_unique<BezierSurfaceC2>(mCtx.cursorPos, std::move(*mBezierPatchCreator), mpRenderer);
                }

                mpRenderer->addRenderable(std::move(pRenderable));

                mBezierPatchCreator.reset();

                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Close"))
            {
                mBezierPatchCreator.reset();

                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (mIsOpenStereoscopyPopup)
        {
            ImGui::OpenPopup("Stereoscopy Settings");
        }

        if (ImGui::BeginPopup("Stereoscopy Settings"))
        {
            ImGui::InputFloat("EyeDistance: ", &mStereoscopySettings.eyeDistance);
            ImGui::InputFloat("FocusPlane: ", &mStereoscopySettings.focusPlane);

            if (ImGui::Button("Enter"))
            {
                ImGui::CloseCurrentPopup();
                mIsOpenStereoscopyPopup = false;
            }

            ImGui::EndPopup();
        }

        ImGui::Spacing();

        if (ImGui::Button("Clear Demo"))
        {
            mpRenderer->clearRenderables();
            mCtx.renderableSelection.renderableIds.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("Unselect All"))
        {
            applyRenderableRotations();
            mCtx.renderableSelection.renderableIds.clear();
        }

        ImGui::Separator();
        ImGui::Spacing();

        ImGuiStyle& style = ImGui::GetStyle();

        ImVec4 backgroundColor = style.Colors[ImGuiCol_WindowBg];
        backgroundColor.z *= 2.2f;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, backgroundColor);
        const float renderablesHeight = mpWindow->getHeight() * renderablesListHeight;

        ImGui::Text("Renderables:");
        ImGui::BeginChild("Scrolling", ImVec2{menuWidth * 0.9f, renderablesHeight});

        int newSelectedItemIndex = -1;
        for (int i = 0; i < mpRenderer->getRenderables().size(); ++i)
        {
            const auto& pRenderable = mpRenderer->getRenderables()[i];

            if (pRenderable == nullptr)
            {
                continue;
            }

            if (ImGui::Selectable(pRenderable->mTag.c_str(), mCtx.renderableSelection.renderableIds.contains(pRenderable->getId())))
            {
                newSelectedItemIndex = i;
                mCtx.isUiClicked |= ImGui::IsItemActive();
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered())
        {
            mCtx.isMenuHovered = true;
        }

        if (newSelectedItemIndex != -1)
        {
            applyRenderableRotations();

            // TODO: add unselection with shift pressed
            if (mCtx.isShiftPressed && (mCtx.lastSelectedRenderableId != invalidId))
            {
                auto it = std::ranges::find_if(mpRenderer->getRenderables(), [this](const auto& pRenderable) {
                    return (pRenderable != nullptr) ? (pRenderable->getId() == mCtx.lastSelectedRenderableId) : false;
                });

                if (it != mpRenderer->getRenderables().end())
                {
                    const size_t startIndex = std::distance(mpRenderer->getRenderables().begin(), it);
                    const size_t endIndex = newSelectedItemIndex;

                    const size_t minIndex = std::min(startIndex, endIndex);
                    const size_t maxIndex = std::max(startIndex, endIndex);

                    for (size_t idx = minIndex; idx <= maxIndex; ++idx)
                    {
                        const auto& pRenderable = mpRenderer->getRenderables().at(idx);
                        if (pRenderable != nullptr)
                        {
                            mCtx.renderableSelection.renderableIds.insert(pRenderable->getId());
                            mSelectedBernsteinPointIndex = -1;
                        }
                    }
                }
            }
            else
            {
                const auto newId = mpRenderer->getRenderables().at(newSelectedItemIndex)->getId();
                if (!mCtx.renderableSelection.renderableIds.contains(newId))
                {
                    if (!mCtx.isCtrlClicked)
                    {
                        mCtx.renderableSelection.renderableIds.clear();
                    }

                    mCtx.renderableSelection.renderableIds.insert(newId);
                    mCtx.lastSelectedRenderableId = newId;
                    mSelectedBernsteinPointIndex = -1;
                }
                else
                {
                    mCtx.renderableSelection.renderableIds.erase(newId);
                    mSelectedBernsteinPointIndex = -1;
                }
            }
        }

        if (mCtx.renderableSelection.renderableIds.size() > 0)
        {
            ImGui::Separator();
            ImGui::Spacing();
        }

        if (mCtx.renderableSelection.renderableIds.size() == 1)
        {
            const Id selectedRenderableId = *mCtx.renderableSelection.renderableIds.begin();

            bool dataChanged{};
            IRenderable* pSelectedRenderable = mpRenderer->getRenderable(selectedRenderableId);
            ASSERT(pSelectedRenderable != nullptr);

            ImGui::Text("Selected item: %s", pSelectedRenderable->mTag.c_str());

            static char nameBuffer[256]{};

            if (newSelectedItemIndex != -1)
            {
                strncpy_s(nameBuffer, pSelectedRenderable->mTag.c_str(), sizeof(nameBuffer) - 1);
                nameBuffer[sizeof(nameBuffer) - 1] = '\0';
            }

            dataChanged |= ImGui::InputText("##tag", nameBuffer, sizeof(nameBuffer));
            mCtx.isUiClicked |= ImGui::IsItemActive();

            ImGui::SameLine();

            if (ImGui::Button("Rename"))
            {
                pSelectedRenderable->mTag = nameBuffer;
            }

            ImGui::Text("Entity ID: %d", pSelectedRenderable->getId());

            ImGui::Spacing();

            float localPos[3]{};
            float worldPos[3]{};
            float rotation[3]{};
            float scale[3]{};

            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(localPos), pSelectedRenderable->mLocalPos);
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(worldPos), pSelectedRenderable->mWorldPos);
            rotation[0] = pSelectedRenderable->mPitch;
            rotation[1] = pSelectedRenderable->mYaw;
            rotation[2] = pSelectedRenderable->mRoll;
            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(scale), pSelectedRenderable->mScale);

            ImGui::Text("Local Position:");
            if (ImGui::DragFloat3("##localPos", localPos))
            {
                pSelectedRenderable->mLocalPos = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(localPos));
                pSelectedRenderable->mWorldPos = pSelectedRenderable->mLocalPos - mCtx.cursorPos;
            }
            mCtx.isUiClicked |= ImGui::IsItemActive();

            ImGui::Text("World Position:");
            if (ImGui::DragFloat3("##worldPos", worldPos))
            {
                pSelectedRenderable->mWorldPos = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(worldPos));
                pSelectedRenderable->mLocalPos = pSelectedRenderable->mWorldPos - mCtx.cursorPos;
            }
            mCtx.isUiClicked |= ImGui::IsItemActive();

            ImGui::Text("Rotation:");
            if (ImGui::DragFloat3("##rotation", rotation))
            {
                pSelectedRenderable->mPitch = rotation[0];
                pSelectedRenderable->mYaw = rotation[1];
                pSelectedRenderable->mRoll = rotation[2];
            }
            mCtx.isUiClicked |= ImGui::IsItemActive();

            if (pSelectedRenderable->isScalable())
            {
                ImGui::BeginDisabled();
            }

            ImGui::Text("Scale:");
            if (ImGui::DragFloat3("##scale", scale))
            {
                pSelectedRenderable->mScale = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(scale));
            }
            mCtx.isUiClicked |= ImGui::IsItemActive();

            if (pSelectedRenderable->isScalable())
            {
                ImGui::EndDisabled();
            }
            ImVec2 dragSize = ImGui::GetItemRectSize();

            // TODO: color picker
            ImVec4 color = ImVec4(0.42f, 0.65f, 0.88f, 1.0f);

            if (ImGui::Button("Open Color Picker", dragSize))
            {
                ImGui::OpenPopup("Color Picker Popup");
            }

            if (ImGui::BeginPopupModal("Color Picker Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Adjust color settings:");

                if (ImGui::ColorPicker4("My Color Picker", (float*)&color))
                {
                }

                if (ImGui::Button("Close"))
                {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            if (auto pTorus = dynamic_cast<Torus*>(pSelectedRenderable); pTorus != nullptr)
            {
                ImGui::Text("Major Radius:");
                dataChanged |= ImGui::SliderFloat("##majorRadius", &pTorus->mMajorRadius, 0.0f, 1.0f);
                mCtx.isUiClicked |= ImGui::IsItemActive();

                ImGui::Text("Minor Radius:");
                dataChanged |= ImGui::SliderFloat("##minorRadius", &pTorus->mMinorRadius, 0.0f, 1.0f);
                mCtx.isUiClicked |= ImGui::IsItemActive();

                ImGui::Spacing();

                ImGui::Text("Major Segments:");
                dataChanged |= ImGui::SliderInt("##majorSegments", &pTorus->mMajorSegments, 3, 100);
                mCtx.isUiClicked |= ImGui::IsItemActive();

                ImGui::Text("Minor Segments:");
                dataChanged |= ImGui::SliderInt("##minorSegments", &pTorus->mMinorSegments, 3, 100);
                mCtx.isUiClicked |= ImGui::IsItemActive();
            }
            else if (auto pPoint = dynamic_cast<Point*>(pSelectedRenderable); pPoint != nullptr)
            {
                ImGui::Text("Radius:");
                dataChanged |= ImGui::SliderFloat("##radius", &pPoint->mRadius, 0.1f, 10.0f);
                mCtx.isUiClicked |= ImGui::IsItemActive();

                ImGui::Text("Segments:");
                dataChanged |= ImGui::SliderInt("##segments", &pPoint->mSegments, 1, 100);
                mCtx.isUiClicked |= ImGui::IsItemActive();
            }
            else if (auto pBezier = dynamic_cast<IBezierCurve*>(pSelectedRenderable); pBezier != nullptr)
            {
                if (dynamic_cast<InterpolatedBezierCurveC2*>(pSelectedRenderable) == nullptr)
                {
                    ImGui::Checkbox("Toggle Polygon", &pBezier->mIsPolygon);
                }

                ImGui::Text("Control points:");
                if (ImGui::TreeNode("de Boor points"))
                {
                    Id idToDelete = invalidId;
                    Id idToEdit = invalidId;
                    for (const Id id : pBezier->mControlPointIds)
                    {
                        if (ImGui::Button(("X##" + std::to_string(id)).c_str()))
                        {
                            idToDelete = id;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(("Edit##" + std::to_string(id)).c_str()))
                        {
                            idToEdit = id;
                        }

                        ImGui::SameLine();
                        ImGui::Text("- %s", pDX11Renderer->getRenderable(id)->mTag.c_str());
                    }

                    if (idToDelete != invalidId)
                    {
                        pBezier->mControlPointIds.erase(
                            std::ranges::remove(pBezier->mControlPointIds, idToDelete).begin(),
                            pBezier->mControlPointIds.end());
                        pBezier->regenerateData();
                    }

                    if (idToEdit != invalidId)
                    {
                        mCtx.renderableSelection.renderableIds.clear();
                        mCtx.renderableSelection.renderableIds.insert(idToEdit);
                    }

                    ImGui::TreePop();
                }

                if (auto pBezierC2 = dynamic_cast<BezierCurveC2*>(pBezier); pBezierC2 != nullptr)
                {
                    if (ImGui::TreeNode("Bernstein points"))
                    {
                        for (int idx = 0; idx < static_cast<size_t>(pBezierC2->getBernsteinPositions().size()); ++idx)
                        {
                            bool isThisBernsteinPointSelected = mSelectedBernsteinPointIndex == idx;
                            const char* buttonName =
                                !isThisBernsteinPointSelected ? "Edit##bernstein" : "Stop Edit##bernstein";

                            if (isThisBernsteinPointSelected)
                            {
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{1.0f, 0.0f, 0.0f, 1.0f});
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{1.0f, 0.2f, 0.2f, 1.0f});
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{1.0f, 0.2f, 0.2f, 1.0f});
                            }

                            if (isBernsteinEdited)
                            {
                                ImGui::EndDisabled();
                            }

                            if (ImGui::Button((buttonName + std::to_string(idx)).c_str()))
                            {
                                if (!isThisBernsteinPointSelected)
                                {
                                    mSelectedBernsteinPointIndex = idx;
                                }
                                else
                                {
                                    mSelectedBernsteinPointIndex = -1;
                                }
                            }

                            if (isBernsteinEdited)
                            {
                                ImGui::BeginDisabled();
                            }

                            if (isThisBernsteinPointSelected)
                            {
                                ImGui::PopStyleColor(3);
                            }

                            ImGui::SameLine();
                            ImGui::Text("- Point %d", idx);
                        }

                        ImGui::TreePop();
                    }
                }
            }
            else if (auto pBezierSurface = dynamic_cast<IBezierSurface*>(pSelectedRenderable))
            {
                ImGui::Text("TesFactor:");
                dataChanged |= ImGui::SliderInt("##tesFactor", &pSelectedRenderable->mTesFactor, 2, 64);
                mCtx.isUiClicked |= ImGui::IsItemActive();

                if (auto pBezierSurfaceC2 = dynamic_cast<BezierSurfaceC2*>(pBezierSurface))
                {
                    ImGui::Checkbox("Toggle Polygon", &pBezierSurfaceC2->mIsPolygon);
                }
            }

            if (dataChanged)
            {
                pSelectedRenderable->regenerateData();
                pDX11Renderer->buildGeometryBuffers();
            }

            mCtx.isUiClicked |= dataChanged;
        }
        else if (mCtx.renderableSelection.renderableIds.size() > 1)
        {
            std::vector<Id> deBoorIds;

            for (const auto id : mCtx.renderableSelection.renderableIds)
            {
                IRenderable* pSelectedRenderable = mpRenderer->getRenderable(id);
                if (dynamic_cast<Point*>(pSelectedRenderable) != nullptr)
                {
                    deBoorIds.push_back(id);
                }
            }

            if (deBoorIds.size() >= 3)
            {
                bool isBezierAdded = false;
                if (ImGui::Button("Add Bezier C0"))
                {
                    auto pBezier = std::make_unique<BezierCurveC0>(deBoorIds, mpRenderer);
                    pBezier->regenerateData();
                    mpRenderer->addRenderable(std::move(pBezier));
                    isBezierAdded = true;
                }
                ImGui::SameLine();
                if ((deBoorIds.size() >= 4) && ImGui::Button("Add Bezier C2"))
                {
                    auto pBezier = std::make_unique<BezierCurveC2>(deBoorIds, mpRenderer);
                    pBezier->regenerateData();
                    mpRenderer->addRenderable(std::move(pBezier));
                    isBezierAdded = true;
                }
                if (ImGui::Button("Add InterpolatedBezier C2")) // TODO: try to keep buttons in the same line
                {
                    auto pBezier = std::make_unique<InterpolatedBezierCurveC2>(deBoorIds, mpRenderer);
                    pBezier->regenerateData();
                    mpRenderer->addRenderable(std::move(pBezier));
                    isBezierAdded = true;
                }

                if (isBezierAdded)
                {
                    mCtx.renderableSelection.renderableIds.clear();
                }
            }

            float centroidPos[3]{};

            XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(centroidPos), mCtx.centroidPos);

            ImGui::Text("Centroid position:");
            if (ImGui::DragFloat3("##centroidPos", centroidPos))
            {
                mCtx.centroidPos = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(centroidPos));
            }
            mCtx.isUiClicked |= ImGui::IsItemActive();

            ImGui::Text("Centroid Pitch:");
            ImGui::DragFloat("##centroidPitch", &mCtx.centroidPitch);
            mCtx.isUiClicked |= ImGui::IsItemActive();

            ImGui::Text("Centroid Yaw:");
            ImGui::DragFloat("##centroidYaw", &mCtx.centroidYaw);
            mCtx.isUiClicked |= ImGui::IsItemActive();

            ImGui::Text("Centroid Roll:");
            ImGui::DragFloat("##centroidRoll", &mCtx.centroidRoll);
            mCtx.isUiClicked |= ImGui::IsItemActive();

            ImGui::Text("Centroid Scale:");
            ImGui::DragFloat("##centroidScale", &mCtx.centroidScale);
            mCtx.isUiClicked |= ImGui::IsItemActive();

            if (std::vector<Point*> selectedPoints = mCtx.renderableSelection.getRenderables<Point>(mpRenderer); selectedPoints.size() == 2)
            {
                if (ImGui::Button("Collapse points"))
                {
                    
                }
            }
        }

        if (isBernsteinEdited)
        {
            ImGui::EndDisabled();
        }
    }

    ImGui::End();
}

void CADDemo::update(float dt)
{
    IDemo::update(dt);

    auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);
}

void CADDemo::drawBernsteins(const std::vector<XMFLOAT3>& bernsteinPositions)
{
    auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

    pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().billboardVS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->HSSetShader(nullptr, nullptr, 0);
    pDX11Renderer->getContext()->DSSetShader(nullptr, nullptr, 0);
    pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
    pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().billboardPS.first.Get(), nullptr, 0);

    long long int currentBernsteinIndex = 0;
    for (const XMFLOAT3& bernsteinPos : bernsteinPositions)
    {
        const XMVECTOR bernsteinVec = XMLoadFloat3(&bernsteinPos);
        pDX11Renderer->mGlobalCB.modelMtx =
            XMMatrixScaling(0.05f, 0.05f, 0.05f) * XMMatrixTranslationFromVector(bernsteinVec);
        if ((currentBernsteinIndex == mSelectedBernsteinPointIndex) ||
            ((currentBernsteinIndex == mSelectedBernsteinPointIndex - 1) && (mSelectedBernsteinPointIndex % 4 == 0)))
        {
            pDX11Renderer->mGlobalCB.color = defaultSelectionColor;
        }
        else
        {
            pDX11Renderer->mGlobalCB.color = defaultColor;
        }
        pDX11Renderer->updateCB(pDX11Renderer->mGlobalCB);
        pDX11Renderer->getContext()->Draw(6, 0); // TODO: drawInstanced
        currentBernsteinIndex++;
    }
}

// TODO: create abstraction
void openFileDialog(fs::path& path, std::function<void()> pResumeProcessing, std::function<void()> pStopProcessing)
{
    IFileOpenDialog* pFileOpen;
    HR(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog,
                            reinterpret_cast<void**>(&pFileOpen)));

    IShellItem* pDefaultFolder;
    HR(SHCreateItemFromParsingName(L"assets/scenes", NULL,
                                   IID_PPV_ARGS(&pDefaultFolder)));
    pFileOpen->SetDefaultFolder(pDefaultFolder);
    pDefaultFolder->Release();

    // TODO: I think that returned paths can be better handled
    pStopProcessing();
    PWSTR pszFilePath{};
    if (SUCCEEDED(pFileOpen->Show(NULL)))
    {
        IShellItem* pItem;
        HR(pFileOpen->GetResult(&pItem));
        HR(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath));
        pItem->Release();
    }
    pResumeProcessing();

    if (pszFilePath != nullptr)
    {
        path = pszFilePath;

        CoTaskMemFree(pszFilePath);

        ASSERT(fs::exists(path) && fs::is_regular_file(path));
    }

    pFileOpen->Release();
    CoUninitialize();
}


void CADDemo::saveScene()
{
    MG1::SceneSerializer serializer;
    MG1::Scene& scene = MG1::Scene::Get();

    for (const auto& pRenderable : mpRenderer->getRenderables())
    {
        if ((pRenderable == nullptr) || (pRenderable->getId() == invalidId))
        {
            // So far I don't expect renderable with invalid id 
            ASSERT(pRenderable->getId() != invalidId);
            continue;
        }

        if (auto pPoint = dynamic_cast<Point*>(pRenderable.get()))
        {
            MG1::Point serializerPoint;
            serializerPoint.SetId(static_cast<uint32_t>(pPoint->getId()));
            serializerPoint.name = pPoint->mTag;
            serializerPoint.position = MG1::Float3{
                XMVectorGetX(pPoint->mWorldPos),
                XMVectorGetY(pPoint->mWorldPos),
                XMVectorGetZ(pPoint->mWorldPos)
            };
            scene.points.emplace_back(std::move(serializerPoint));
        }
        else if (auto pTorus = dynamic_cast<Torus*>(pRenderable.get()))
        {
            MG1::Torus serializerTorus;
            serializerTorus.SetId(static_cast<uint32_t>(pTorus->getId()));
            serializerTorus.name = pTorus->mTag;
            serializerTorus.position = MG1::Float3{
                XMVectorGetX(pTorus->mWorldPos),
                XMVectorGetY(pTorus->mWorldPos),
                XMVectorGetZ(pTorus->mWorldPos)
            };
            serializerTorus.rotation = MG1::Float3{
                pTorus->mPitch,
                pTorus->mYaw,
                pTorus->mRoll
            };
            serializerTorus.scale = MG1::Float3{
                XMVectorGetX(pTorus->mScale),
                XMVectorGetY(pTorus->mScale),
                XMVectorGetZ(pTorus->mScale)
            };
            serializerTorus.samples = MG1::Uint2{
                static_cast<uint32_t>(pTorus->mMajorSegments),
                static_cast<uint32_t>(pTorus->mMinorSegments),
            };
            serializerTorus.largeRadius = pTorus->mMajorRadius;
            serializerTorus.smallRadius = pTorus->mMinorRadius;

            scene.tori.emplace_back(std::move(serializerTorus));
        }
        else if (auto pBezierCurveC0 = dynamic_cast<BezierCurveC0*>(pRenderable.get()))
        {
            //MG1::BezierC0 serializerBezierCurveC0;

            //serializerBezierCurveC0.SetId(static_cast<uint32_t>(pTorus->getId()));
            //serializerBezierCurveC0.name = pTorus->mTag;
            //serializerBezierCurveC0.controlPoints.insert(
            //    reinterpret_cast<MG1::PointRef*>(pBezierCurveC0->mControlPointIds.begin()),
            //    reinterpret_cast<MG1::PointRef*>(pBezierCurveC0->mControlPointIds.end()));

            //scene.bezierC0.emplace_back(std::move(serializerBezierCurveC0));
        }
        else if (auto pBezierCurveC2 = dynamic_cast<BezierCurveC2*>(pRenderable.get()))
        {

        }
        else if (auto pInterpolatedBezierCurveC2 = dynamic_cast<InterpolatedBezierCurveC2*>(pRenderable.get()))
        {

        }
        else if (auto pBezierSurfaceC0 = dynamic_cast<BezierSurfaceC0*>(pRenderable.get()))
        {

        }
        else if (auto pBezierSurfaceC2 = dynamic_cast<BezierSurfaceC2*>(pRenderable.get()))
        {

        }
        else
        {
            ASSERT(false);
        }
    }

    try
    {
        if (!scene.IsValid())
        {
            throw std::runtime_error{"Scene is not valid"};
        }
        // TODO: input save file system and location
        serializer.SaveScene("build/saved_scene.json");
    }
    catch (const std::exception& e)
    {
        ERR(std::format("EXCEPTION WHILE SAVING SCENE FILE: {}", e.what()).c_str());
    }
    catch (...)
    {
        ERR("UNSPECIFIED EXCEPTION WHILE SAVING SCENE�FILE");
    }
}

void CADDemo::loadScene()
{
    fs::path scenePath;
    openFileDialog(scenePath, std::bind(&IWindow::resumeProcessing, mpWindow.get()),
                   std::bind(&IWindow::stopProcessing, mpWindow.get()));

    if (scenePath.empty())
    {
        return;
    }

    MG1::SceneSerializer serializer;

    try
    {
        serializer.LoadScene(scenePath.c_str());
    }
    catch (const std::exception& e)
    {
        ERR(std::format("EXCEPTION WHILE LOADING SCENE FILE: {}", e.what()).c_str());
    }
    catch (...)
    {
        ERR("UNSPECIFIED EXCEPTION WHILE LOADING SCENE FILE");
    }

    for (auto& pRenderable : mpRenderer->getRenderables())
    {
        mpRenderer->removeRenderable(pRenderable->getId());
    }

    const MG1::Scene& scene = MG1::Scene::Get();

    for (const MG1::Point& serialPoint : scene.points)
    {
        auto pPoint = std::make_unique<Point>();
        pPoint->applySerializerData(serialPoint);
        mpRenderer->addRenderable(std::move(pPoint));
    }

    for (const MG1::Torus& serialTorus : scene.tori)
    {
        auto pTorus = std::make_unique<Torus>();
        pTorus->applySerializerData(serialTorus);
        mpRenderer->addRenderable(std::move(pTorus));
    }

    const auto serializeBezier = [this](const MG1::Bezier& serialBezier) -> void {
        std::vector<Id> ids;
        ids.reserve(serialBezier.controlPoints.size());

        std::ranges::transform(serialBezier.controlPoints, std::back_inserter(ids), [](const MG1::PointRef point) -> Id {
            return static_cast<Id>(point.GetId());
        });

        auto pBezierCurveC0 = std::make_unique<BezierCurveC0>(ids, mpRenderer);
        pBezierCurveC0->applySerializerData(serialBezier);
        mpRenderer->addRenderable(std::move(pBezierCurveC0));
    };

    for (const MG1::Bezier& serialBezierC0 : scene.bezierC0)
    {
        serializeBezier(serialBezierC0);
    }

    for (const MG1::Bezier& serialBezierC2 : scene.bezierC2)
    {
        serializeBezier(serialBezierC2);
    }

    for (const MG1::Bezier& serialinterpolatedBezierC2: scene.interpolatedC2)
    {
        serializeBezier(serialinterpolatedBezierC2);
    }

    for (const MG1::BezierSurfaceC0& serialBezierSurfaceC0 : scene.surfacesC0)
    {
        ASSERT(!serialBezierSurfaceC0.patches.empty());

        const BezierSurfaceCreator bezierSurfaceCreator
        {
            .u = serialBezierSurfaceC0.size.x,
            .v = serialBezierSurfaceC0.size.y,
            .isWrapped = serialBezierSurfaceC0.uWrapped && serialBezierSurfaceC0.vWrapped,
            .isC2 = false,
        };

        auto pBezierSurfaceC0 = std::make_unique<BezierSurfaceC0>(mCtx.cursorPos, std::move(bezierSurfaceCreator), mpRenderer);
        pBezierSurfaceC0->applySerializerData(serialBezierSurfaceC0, serialBezierSurfaceC0.patches);
        mpRenderer->addRenderable(std::move(pBezierSurfaceC0));
    }

    for (const MG1::BezierSurfaceC2& serialBezierSurfaceC2 : scene.surfacesC2)
    {
        ASSERT(!serialBezierSurfaceC2.patches.empty());

        const BezierSurfaceCreator bezierSurfaceCreator
        {
            .u = serialBezierSurfaceC2.size.x,
            .v = serialBezierSurfaceC2.size.y,
            .isWrapped = serialBezierSurfaceC2.uWrapped && serialBezierSurfaceC2.vWrapped,
            .isC2 = true,
        };

        auto pBezierSurfaceC2 = std::make_unique<BezierSurfaceC2>(mCtx.cursorPos, std::move(bezierSurfaceCreator), mpRenderer);
        pBezierSurfaceC2->applySerializerData(serialBezierSurfaceC2, serialBezierSurfaceC2.patches);
        mpRenderer->addRenderable(std::move(pBezierSurfaceC2));
    }
}
