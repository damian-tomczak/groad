module;

#include "utils.h"

#include "imgui.h"

#include "mg.hpp"

export module cad_demo;
export import core.demo;
import core.context;
import core.renderables;

export class CADDemo : public IDemo
{
public:
    CADDemo(Context& ctx, IRenderer* pRenderer, std::shared_ptr<IWindow> pWindow);

    void init() override;
    void update(float dt) override;
    void draw(GlobalCB& cb) override;
    void processInput(IWindow::Message msg, float dt) override;
    void renderUi() override;

private:
    std::optional<int> mLastXMousePosition;
    std::optional<int> mLastYMousePosition;
};

class GridSurface : public ISurface
{
public:
    GridSurface(IRenderer* pRenderer) : ISurface{pRenderer}
    {
    }

    void init() override{};
    void draw(GlobalCB& cb) override
    {
        DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it
        ID3D11DeviceContext* const pContext = pDX11Renderer->getContext();

        pContext->VSSetShader(pDX11Renderer->getShaders().gridVS.first.Get(), nullptr, 0);
        pContext->PSSetShader(pDX11Renderer->getShaders().gridPS.first.Get(), nullptr, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pContext->Draw(6, 0);
    }

    void update(float dt) override
    {
    }
};

module :private;

CADDemo::CADDemo(Context& ctx, IRenderer* pRenderer, std::shared_ptr<IWindow> pWindow)
    : IDemo{ctx, "CADDemo", pRenderer, pWindow, std::make_unique<GridSurface>(pRenderer)}
{
    std::unordered_set<IRenderable::Id> selectedPointsIds;

    XMVECTOR pos{-2.f, 3.f, 0};
    auto pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{-1.f, 3.f, 0};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{0.0f, 3.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{1.0f, 3.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{-2.0f, 1.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{-1.0f, 1.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{0.0f, 1.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{1.0f, 1.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    selectedPointsIds.insert(pPoint->mId);
    mpRenderer->addRenderable(std::move(pPoint));

    auto pBezier = std::make_unique<BezierC0>(selectedPointsIds, mpRenderer);
    pBezier->regenerateData();
    mpRenderer->addRenderable(std::move(pBezier));
}

void CADDemo::init()
{
    IDemo::init();
}

void CADDemo::draw(GlobalCB& cb)
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it
    ID3D11DeviceContext* const pContext = pDX11Renderer->getContext();

    mpSurface->draw(cb);

    for (const auto [renderableIdx, pRenderable] : std::views::enumerate(pDX11Renderer->mRenderables))
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

        if (mCtx.selectedRenderableIds.contains(pRenderable->mId))
        {
            XMMATRIX pivotPitchRotationMat = XMMatrixRotationX(mCtx.pivotPitch);
            XMMATRIX pivotYawRotationMat = XMMatrixRotationY(mCtx.pivotYaw);
            XMMATRIX pivotRollRotationMat = XMMatrixRotationZ(mCtx.pivotRoll);

            pivotRotationMat = pivotPitchRotationMat * pivotYawRotationMat * pivotRollRotationMat;

            pivotScaleMat = XMMatrixScaling(mCtx.pivotScale, mCtx.pivotScale, mCtx.pivotScale);

            translationToOrigin = XMMatrixTranslationFromVector(-(mCtx.pivotPos - pRenderable->mWorldPos));
            translationBack = XMMatrixTranslationFromVector((mCtx.pivotPos - pRenderable->mWorldPos));
        }

        XMMATRIX worldTranslation = XMMatrixTranslationFromVector(pRenderable->mWorldPos);

        // clang-format off
        model = localScaleMat * localRotationMat *
                translationToOrigin * pivotScaleMat * pivotRotationMat * translationBack *
                worldTranslation;
        // clang-format on

        cb.modelMtx = model;

        cb.flags =
            (std::ranges::find(mCtx.selectedRenderableIds, pRenderable->mId) != mCtx.selectedRenderableIds.end()) ? 1
                                                                                                                    : 0;

        pDX11Renderer->updateCB(cb);

        unsigned vStride;
        unsigned offset = 0;
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
            pDX11Renderer->getContext()->IASetInputLayout(pDX11Renderer->getDefaultInputLayout());
            pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().defaultVS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->HSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->DSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().defaultPS.first.Get(), nullptr, 0);
        }
        else
        {
            pDX11Renderer->getContext()->IASetInputLayout(pDX11Renderer->getBezierInputLayout());
            pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().bezierC0VS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->HSSetShader(pDX11Renderer->getShaders().bezierC0HS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->DSSetShader(pDX11Renderer->getShaders().bezierC0DS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().bezierC0PS.first.Get(), nullptr, 0);
        }

        pDX11Renderer->getContext()->IASetVertexBuffers(
            0, 1, pDX11Renderer->mVertexBuffers.at(renderableIdx).GetAddressOf(), &vStride, &offset);
        const ComPtr<ID3D11Buffer> indexBuffer = pDX11Renderer->mIndexBuffers.at(renderableIdx);
        if (indexBuffer != nullptr)
        {
            pDX11Renderer->getContext()->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        }

        if (dynamic_cast<Point*>(pRenderable.get()) != nullptr)
        {
            pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        }
        else if (dynamic_cast<IBezier*>(pRenderable.get()) != nullptr)
        {
            pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
        }
        else
        {
            pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        }

        if (auto pBezier = dynamic_cast<IBezier*>(pRenderable.get()); pBezier == nullptr)
        {
            pDX11Renderer->getContext()->DrawIndexed(static_cast<UINT>(pRenderable->getTopology().size()), 0, 0);
        }
        else
        {
            const auto& controlPoints = pBezier->mControlPointRenderableIds;
            const auto controlPointsSize = controlPoints.size();
            for (unsigned i = 0; i < controlPointsSize + 1; i += IBezier::controlPointsNumber)
            {
                offset = i * sizeof(XMFLOAT3);
                pDX11Renderer->getContext()->IASetVertexBuffers(
                    0, 1, pDX11Renderer->mVertexBuffers.at(renderableIdx).GetAddressOf(), &vStride, &offset);

                pDX11Renderer->getContext()->Draw(1, 0);

                if (pBezier->mIsPolygon)
                {
                    pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
                    pDX11Renderer->getContext()->HSSetShader(nullptr, nullptr, 0);
                    pDX11Renderer->getContext()->DSSetShader(nullptr, nullptr, 0);
                    pDX11Renderer->getContext()->GSSetShader(pDX11Renderer->getShaders().bezierC0BorderGS.first.Get(),
                                                             nullptr, 0);
                    int tmpFlags = cb.flags;
                    cb.flags = 2;

                    pDX11Renderer->updateCB(cb);

                    pDX11Renderer->getContext()->Draw(1, 0);

                    cb.flags = tmpFlags;
                    pDX11Renderer->getContext()->IASetPrimitiveTopology(
                        D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
                    pDX11Renderer->getContext()->HSSetShader(pDX11Renderer->getShaders().bezierC0HS.first.Get(),
                                                             nullptr, 0);
                    pDX11Renderer->getContext()->DSSetShader(pDX11Renderer->getShaders().bezierC0DS.first.Get(),
                                                             nullptr, 0);
                    pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);

                    pDX11Renderer->updateCB(cb);
                }
            }
        }
    }

    if (mCtx.selectedRenderableIds.size() > 1)
    {
        XMVECTOR sum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        for (const auto& selectedRenderableId : mCtx.selectedRenderableIds)
        {
            const auto pRenderable = pDX11Renderer->getRenderable(selectedRenderableId);
            sum = XMVectorAdd(sum, pRenderable->mLocalPos + pRenderable->mWorldPos);
        }

        float numPositions = static_cast<float>(mCtx.selectedRenderableIds.size());
        mCtx.pivotPos = XMVectorScale(sum, 1.0f / numPositions);

        auto pPoint = std::make_unique<Point>(mCtx.pivotPos, 0.1f, 25, false);
        pPoint->regenerateData();
        IRenderable::Id mId = pPoint->mId;
        pDX11Renderer->addRenderable(std::move(pPoint));
        pPoint.reset();

        pDX11Renderer->buildGeometryBuffers();

        const XMMATRIX translationMat = XMMatrixTranslationFromVector(mCtx.pivotPos);
        cb.modelMtx = translationMat;
        cb.flags = 2;

        pDX11Renderer->updateCB(cb);

        UINT vStride = sizeof(XMFLOAT3), offset = 0;
        pDX11Renderer->getContext()->IASetVertexBuffers(0, 1, &pDX11Renderer->mVertexBuffers.back(), &vStride, &offset);
        pDX11Renderer->getContext()->IASetIndexBuffer(pDX11Renderer->mIndexBuffers.back().Get(), DXGI_FORMAT_R32_UINT,
                                                      0);

        pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().defaultVS.first.Get(), nullptr, 0);
        pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().defaultPS.first.Get(), nullptr, 0);
        pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        auto pTest = pDX11Renderer->getRenderable(mId);
        pDX11Renderer->getContext()->DrawIndexed(static_cast<UINT>(pTest->getTopology().size()), 0, 0);

        pDX11Renderer->removeRenderable(mId);
    }
}

void CADDemo::processInput(IWindow::Message msg, float dt)
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

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
                    mCtx.selectedRenderableIds.insert(pRenderable->mId);
                }
            }
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

        if (mCtx.isLeftMouseClicked && (!mCtx.isUiClicked) && !(mCtx.selectedRenderableIds.empty()))
        {
            const float pitchOffset = yOffset * 0.2f;
            const float yawOffset = xOffset * 0.2f;

            if ((mCtx.selectedRenderableIds.size() > 1) && (mCtx.interactionType == InteractionType::ROTATE))
            {
                mCtx.pivotPitch += pitchOffset;
                mCtx.pivotYaw += yawOffset;
                break;
            }

            if ((mCtx.selectedRenderableIds.size() > 1) && (mCtx.interactionType == InteractionType::SCALE))
            {
                mCtx.pivotScale += -yOffset * 0.1f;
                break;
            }

            for (const IRenderable::Id selectedRenderableId : mCtx.selectedRenderableIds)
            {
                IRenderable* const pRenderable = mpRenderer->getRenderable(selectedRenderableId);

                switch (mCtx.interactionType)
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
    }
    break;
    case IWindow::Message::KEY_DELETE_DOWN:
        for (auto it = mCtx.selectedRenderableIds.begin(); it != mCtx.selectedRenderableIds.end();
             it = mCtx.selectedRenderableIds.begin())
        {
            pDX11Renderer->removeRenderable(*it);
            mCtx.selectedRenderableIds.erase(*it);
        }
        for (auto& pRenderable : pDX11Renderer->mRenderables)
        {
            pRenderable->regenerateData();
        }
        break;
    }
}

void CADDemo::renderUi()
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

    mCtx.isMenuEnabled = false;
    mCtx.isUiClicked = false;

    const float menuWidth = mpWindow->getWidth() * 0.2f;

    ImGui::SetNextWindowPos(ImVec2{mpWindow->getWidth() - menuWidth, mCtx.menuBarHeight});
    ImGui::SetNextWindowSize(ImVec2{menuWidth, static_cast<float>(mpWindow->getHeight())});

    if (ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        mCtx.isMenuEnabled = true;
    }

    if (ImGui::IsWindowHovered())
    {
        mCtx.isMenuHovered = true;
    }
    else
    {
        mCtx.isMenuHovered = false;
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
        IBezier* pBezier = nullptr;
        if (mCtx.selectedRenderableIds.size() == 1)
        {
            const IRenderable::Id selectedRenderableId = *mCtx.selectedRenderableIds.begin();
            IRenderable* const pRenderable = mpRenderer->getRenderable(selectedRenderableId);
            pBezier = dynamic_cast<IBezier*>(pRenderable);
        }

        auto pPoint = std::make_unique<Point>(mCtx.cursorPos);
        pPoint->regenerateData();
        const IRenderable::Id pointId = pPoint->mId;
        mpRenderer->addRenderable(std::move(pPoint));

        if (pBezier->mId != IRenderable::invalidId)
        {
            pBezier->insertControlPoint(pointId);
        }
    }

    if (ImGui::Button("Clear Demo"))
    {
        mpRenderer->mRenderables.clear();
    }

    ImGui::Separator();
    ImGui::Spacing();

    ImGuiStyle& style = ImGui::GetStyle();

    ImVec4 backgroundColor = style.Colors[ImGuiCol_WindowBg];
    backgroundColor.z *= 2.2f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, backgroundColor);
    const float renderablesHeight = mpWindow->getHeight() * 0.2f;

    ImGui::Text("Renderables:");
    ImGui::BeginChild("Scrolling", ImVec2{menuWidth * 0.9f, renderablesHeight});

    int newSelectedItemIdx = -1;
    for (int i = 0; i < mpRenderer->mRenderables.size(); ++i)
    {
        const auto& pRenderable = mpRenderer->mRenderables.at(i);

        if (ImGui::Selectable(pRenderable->mTag.c_str(), mCtx.selectedRenderableIds.contains(pRenderable->mId)))
        {
            newSelectedItemIdx = i;
            mCtx.isUiClicked |= ImGui::IsItemActive();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered())
    {
        mCtx.isMenuHovered = true;
    }

    if (newSelectedItemIdx != -1)
    {
        int i = 0;
        for (const auto renderableSelectedId : mCtx.selectedRenderableIds)
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

            if (mCtx.selectedRenderableIds.contains(pRenderable->mId))
            {
                XMMATRIX pivotPitchRotationMat = XMMatrixRotationX(mCtx.pivotPitch);
                XMMATRIX pivotYawRotationMat = XMMatrixRotationY(mCtx.pivotYaw);
                XMMATRIX pivotRollRotationMat = XMMatrixRotationZ(mCtx.pivotRoll);

                pivotRotationMat = pivotPitchRotationMat * pivotYawRotationMat * pivotRollRotationMat;

                pivotScaleMat = XMMatrixScaling(mCtx.pivotScale, mCtx.pivotScale, mCtx.pivotScale);

                translationToOrigin = XMMatrixTranslationFromVector(-(mCtx.pivotPos - pRenderable->mWorldPos));
                translationBack = XMMatrixTranslationFromVector((mCtx.pivotPos - pRenderable->mWorldPos));
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

        mCtx.pivotPitch = 0;
        mCtx.pivotYaw = 0;
        mCtx.pivotRoll = 0;

        mCtx.pivotScale = 1;

        if (mCtx.isShiftPressed && (mCtx.lastSelectedRenderableId != IRenderable::invalidId))
        {
            auto it = std::ranges::find_if(mpRenderer->mRenderables, [this](const auto& pRenderable) {
                return pRenderable->mId == mCtx.lastSelectedRenderableId;
            });

            if (it != mpRenderer->mRenderables.end())
            {
                const size_t startIdx = std::distance(mpRenderer->mRenderables.begin(), it);
                const size_t endIdx = newSelectedItemIdx;

                const size_t minIdx = std::min(startIdx, endIdx);
                const size_t maxIdx = std::max(startIdx, endIdx);

                for (size_t idx = minIdx; idx <= maxIdx; ++idx)
                {
                    const IRenderable::Id id = mpRenderer->mRenderables.at(idx)->mId;
                    mCtx.selectedRenderableIds.insert(id);
                }
            }
        }
        else
        {
            const auto newId = mpRenderer->mRenderables.at(newSelectedItemIdx)->mId;
            if (!mCtx.selectedRenderableIds.contains(newId))
            {
                if (!mCtx.isCtrlClicked)
                {
                    mCtx.selectedRenderableIds.clear();
                }

                mCtx.selectedRenderableIds.insert(newId);
                mCtx.lastSelectedRenderableId = newId;
            }
            else
            {
                mCtx.selectedRenderableIds.erase(newId);
            }
        }
    }

    if (mCtx.selectedRenderableIds.size() > 0)
    {
        ImGui::Separator();
        ImGui::Spacing();
    }

    if (mCtx.selectedRenderableIds.size() == 1)
    {
        const IRenderable::Id selectedRenderableId = *mCtx.selectedRenderableIds.begin();

        bool dataChanged{};
        IRenderable* pSelectedRenderable = mpRenderer->getRenderable(selectedRenderableId);

        ImGui::Text("Selected item: %s", pSelectedRenderable->mTag.c_str());

        static char nameBuffer[256]{};

        if (newSelectedItemIdx != -1)
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

        ImGui::Text("Scale:");
        ImGui::DragFloat("##scale", &pSelectedRenderable->mScale);
        mCtx.isUiClicked |= ImGui::IsItemActive();

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
        else if (auto pBezier = dynamic_cast<IBezier*>(pSelectedRenderable); pBezier != nullptr)
        {
            ImGui::Checkbox("Toggle Polygon", &pBezier->mIsPolygon);

            // ImGui::Text("De Boor control points:");
            // std::vector<std::string> deboorControlPointsNames;
            // std::vector<const char*> deboorControlPointsNamesPtrs;

            // for (size_t i = 0; i < mControlPointRenderableIds.size(); ++i)
            //{
            //     deboorControlPointsNames.emplace_back("Point " + std::to_string(i));
            // }

            // for (const std::string& deboorControlPointName : deboorControlPointsNames)
            //{
            //     deboorControlPointsNamesPtrs.emplace_back(deboorControlPointName.c_str());
            // }

            // int selectedItem = -1;
            // if (ImGui::ListBox("##deboor", &selectedItem, deboorControlPointsNamesPtrs.data(),
            // static_cast<int>(deboorControlPointsNamesPtrs.size()), 4))
            //{
            // }
            // isUiClicked |= ImGui::IsItemActive();

            // ImGui::Text("De Boor control points:");
            // if (ImGui::ListBox("##deboor", &selectedItem, deboorControlPointsNamesPtrs.data(),
            //                    static_cast<int>(deboorControlPointsNamesPtrs.size()), 4))
            //{
            // }

            ImGui::Text("Control points:");
            if (ImGui::TreeNode("de Boor points"))
            {
                IRenderable::Id idToDelete = IRenderable::invalidId;
                size_t i = 0;
                for (const IRenderable::Id id : pBezier->mControlPointRenderableIds)
                {
                    if (ImGui::Button(("X##" + std::to_string(id)).c_str()))
                    {
                        idToDelete = id;
                    }

                    ImGui::SameLine();
                    ImGui::Text("- Point %d", i);
                    i++;
                }

                if (idToDelete != IRenderable::invalidId)
                {
                    pBezier->mControlPointRenderableIds.erase(idToDelete);
                    pBezier->regenerateData();
                }

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Bernstein points"))
            {
                size_t i = 0;
                for (const IRenderable::Id id : pBezier->mControlPointRenderableIds)
                {
                    if (ImGui::Button(("Edit##" + std::to_string(id)).c_str()))
                    {
                        mCtx.selectedRenderableIds.clear();
                        mCtx.selectedRenderableIds.insert(id);
                    }

                    ImGui::SameLine();
                    ImGui::Text("- Point %d", i);
                    i++;
                }

                ImGui::TreePop();
            }
        }

        if (dataChanged)
        {
            pSelectedRenderable->regenerateData();
            pDX11Renderer->buildGeometryBuffers();
        }

        mCtx.isUiClicked |= dataChanged;
    }
    else if (mCtx.selectedRenderableIds.size() > 1)
    {
        std::unordered_set<IRenderable::Id> selectedPointsIds;

        for (const auto mId : mCtx.selectedRenderableIds)
        {
            IRenderable* pSelectedRenderable = mpRenderer->getRenderable(mId);
            if (dynamic_cast<Point*>(pSelectedRenderable) != nullptr)
            {
                selectedPointsIds.insert(mId);
            }
        }

        if (selectedPointsIds.size() >= 3)
        {
            bool isBezierAdded = false;
            if (ImGui::Button("Add IBezier C0"))
            {
                auto pBezier = std::make_unique<BezierC0>(selectedPointsIds, mpRenderer);
                pBezier->regenerateData();
                mpRenderer->addRenderable(std::move(pBezier));
                isBezierAdded = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Add IBezier C2"))
            {
                ASSERT(false);
                isBezierAdded = true;
            }

            if (isBezierAdded)
            {
                mCtx.selectedRenderableIds.clear();
            }
        }

        float pivotPos[3]{};

        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(pivotPos), mCtx.pivotPos);

        ImGui::Text("Pivot position:");
        if (ImGui::DragFloat3("##pivotPos", pivotPos))
        {
            mCtx.pivotPos = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(pivotPos));
        }
        mCtx.isUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Pivot Pitch:");
        ImGui::DragFloat("##pivotPitch", &mCtx.pivotPitch);
        mCtx.isUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Pivot Yaw:");
        ImGui::DragFloat("##pivotYaw", &mCtx.pivotYaw);
        mCtx.isUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Pivot Roll:");
        ImGui::DragFloat("##pivotRoll", &mCtx.pivotRoll);
        mCtx.isUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Pivot Scale:");
        ImGui::DragFloat("##pivotScale", &mCtx.pivotScale);
        mCtx.isUiClicked |= ImGui::IsItemActive();
    }

    ImGui::End();
}

void CADDemo::update(float dt)
{
    IDemo::update(dt);

    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);
}