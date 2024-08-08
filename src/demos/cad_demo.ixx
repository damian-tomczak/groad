module;

#include "utils.h"

#include "imgui.h"

#include "mg.hpp"

export module cad_demo;
export import core.demo;
import core.context;
import core.renderables;
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
    std::optional<BezierPatchCreator> mBezierPatchCreator;

    void drawBernsteins(const std::vector<XMFLOAT3>& bernsteinPoints);

    long long int mSelectedBernsteinPointIdx = -1;

    enum Message
    {
        EMPTY,
        UNABLE_TO_DELETE,
        MESSAGE_COUNT
    };

    static constexpr auto messageSize = sizeof(Message) * 8;

    std::bitset<messageSize> mMessage;

    // clang-format off
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
    // clang-format on
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

    std::vector<Id> deBoorIds;

    XMVECTOR pos{-2.f, 3.f, 0};
    auto pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    deBoorIds.push_back(pPoint->id);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{-1.f, 3.f, 0};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    deBoorIds.push_back(pPoint->id);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{0.0f, 3.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    deBoorIds.push_back(pPoint->id);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{1.0f, 3.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    deBoorIds.push_back(pPoint->id);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{-2.0f, 1.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    deBoorIds.push_back(pPoint->id);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{-1.0f, 1.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    deBoorIds.push_back(pPoint->id);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{0.0f, 1.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    deBoorIds.push_back(pPoint->id);
    mpRenderer->addRenderable(std::move(pPoint));

    pos = XMVECTOR{1.0f, 1.0f, 0.0f};
    pPoint = std::make_unique<Point>(pos);
    pPoint->regenerateData();
    deBoorIds.push_back(pPoint->id);
    mpRenderer->addRenderable(std::move(pPoint));

    //auto pBezierC0 = std::make_unique<BezierC0>(deBoorIds, mpRenderer, Colors::Pink);
    //pBezierC0->regenerateData();
    //mpRenderer->addRenderable(std::move(pBezierC0));

    //auto pBezierC2 = std::make_unique<BezierC2>(deBoorIds, mpRenderer, Colors::Orange);
    //pBezierC2->regenerateData();
    //mpRenderer->addRenderable(std::move(pBezierC2));

    auto pInterpolatedBezierC2 = std::make_unique<InterpolatedBezierC2>(deBoorIds, mpRenderer, Colors::Brown);
    pInterpolatedBezierC2->regenerateData();
    mpRenderer->addRenderable(std::move(pInterpolatedBezierC2));

    BezierPatchCreator bezierPatchCreator{};
    pos = XMVECTOR{2.0f, 3.0f, 0.0f};
    auto pBezierPatchC0 = std::make_unique<BezierPatchC0>(std::move(bezierPatchCreator), pos, mpRenderer);
    pBezierPatchC0->regenerateData();
    mpRenderer->addRenderable(std::move(pBezierPatchC0));
}

CADDemo::~CADDemo()
{
}

void CADDemo::init()
{
    IDemo::init();
}

void CADDemo::draw()
{
    IDemo::draw();

    auto pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

    std::unordered_set<Id> pointIdsPartOfSelected;
    for (const auto selectedRenderableId : mCtx.selectedRenderableIds)
    {
        auto pSelectedRenderable = pDX11Renderer->getRenderable(selectedRenderableId);
        if (auto pIControlPointBased = dynamic_cast<IControlPointBased*>(pSelectedRenderable); pIControlPointBased != nullptr)
        {
            pointIdsPartOfSelected.insert(pIControlPointBased->mControlPointIds.begin(),
                                          pIControlPointBased->mControlPointIds.end());
        }
    }

    for (const auto [renderableIdx, pRenderable] : std::views::enumerate(pDX11Renderer->mRenderables))
    {
        if (pRenderable == nullptr)
        {
            continue;
        }

        pDX11Renderer->mGlobalCB.color = pRenderable->mColor;

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

        if (mCtx.selectedRenderableIds.contains(pRenderable->id))
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

        pDX11Renderer->mGlobalCB.modelMtx = model;

        if (mCtx.selectedRenderableIds.contains(pRenderable->id) || pointIdsPartOfSelected.contains(pRenderable->id))
        {
            pDX11Renderer->mGlobalCB.color = defaultSelectionColor;
        }

        pDX11Renderer->updateCB(pDX11Renderer->mGlobalCB);

        pDX11Renderer->getContext()->IASetVertexBuffers(0, 1,
                                                        pDX11Renderer->mVertexBuffers.at(renderableIdx).GetAddressOf(),
                                                        &pRenderable->getStride(), &pRenderable->getOffset());

        const ComPtr<ID3D11Buffer> indexBuffer = pDX11Renderer->mIndexBuffers.at(renderableIdx);
        if (indexBuffer != nullptr)
        {
            pDX11Renderer->getContext()->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        }

        pRenderable->draw(mpRenderer, renderableIdx);
    }

    if (mCtx.selectedRenderableIds.size() == 1)
    {
        Id selectedRenderableId = *mCtx.selectedRenderableIds.begin();
        IRenderable* pRenderable = pDX11Renderer->getRenderable(selectedRenderableId);
        if (auto pBezierC2 = dynamic_cast<BezierC2*>(pRenderable); pBezierC2 != nullptr)
        {
            drawBernsteins(pBezierC2->getBernsteinPositions());
        }
    }
    else if (mCtx.selectedRenderableIds.size() > 1)
    {
        XMVECTOR sum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        for (const auto& selectedRenderableId : mCtx.selectedRenderableIds)
        {
            const auto pRenderable = pDX11Renderer->getRenderable(selectedRenderableId);
            sum = XMVectorAdd(sum, pRenderable->getGlobalPos());
        }

        float numPositions = static_cast<float>(mCtx.selectedRenderableIds.size());
        mCtx.pivotPos = XMVectorScale(sum, 1.0f / numPositions);

        Point::drawPrimitive(mpRenderer, mCtx.pivotPos);
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
                    mCtx.selectedRenderableIds.insert(pRenderable->id);
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

        if (mCtx.isLeftMouseClicked && (!mCtx.isUiClicked) && (!mCtx.selectedRenderableIds.empty()))
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

            const XMVECTOR offsetVec = XMVectorSet(xOffset * 0.1f, yOffset * 0.1f, 0.0f, 0.0f);

            for (const Id selectedRenderableId : mCtx.selectedRenderableIds)
            {
                IRenderable* const pSelectedRenderable = mpRenderer->getRenderable(selectedRenderableId);

                if (auto pBezierC2 = dynamic_cast<BezierC2*>(pSelectedRenderable); (pBezierC2 != nullptr) && (mSelectedBernsteinPointIdx != -1))
                {
                    long long int idx = 0;
                    for (XMFLOAT3& bernsteinPos : pBezierC2->getBernsteinPositions())
                    {
                        if (idx == mSelectedBernsteinPointIdx)
                        {
                            XMVECTOR va = XMLoadFloat3(&bernsteinPos);
                            const XMVECTOR vc = va + offsetVec;

                            XMStoreFloat3(&bernsteinPos, vc);
                            break;
                        }
                        idx++;
                    }
                }
                else
                {
                    switch (mCtx.interactionType)
                    {
                    case InteractionType::ROTATE:
                        pSelectedRenderable->mPitch += pitchOffset;
                        pSelectedRenderable->mYaw += yawOffset;
                        break;
                    case InteractionType::MOVE:
                    {
                        if (auto pSelectedPoint = dynamic_cast<Point*>(pSelectedRenderable); pSelectedPoint != nullptr)
                        {
                            pSelectedPoint->mWorldPos += offsetVec;

                            for (const auto& pRenderable : mpRenderer->mRenderables)
                            {
                                if (auto pControlPointBased = dynamic_cast<IControlPointBased*>(pRenderable.get());
                                    pControlPointBased != nullptr)
                                {
                                    pRenderable->regenerateData();
                                }
                            }
                        }
                        else if (auto pSelectedBezierC0 = dynamic_cast<BezierC0*>(pSelectedRenderable); pSelectedBezierC0 != nullptr)
                        {
                            // Selected bezier C0
                            // TODO: update only beziers containing modified control points
                            for (const Id controlPointRenderableId : pSelectedBezierC0->mControlPointIds)
                            {
                                IRenderable* const pControlPointRenderable =
                                    mpRenderer->getRenderable(controlPointRenderableId);

                                pControlPointRenderable->mWorldPos += offsetVec;
                            }

                            for (const auto& pRenderable : mpRenderer->mRenderables)
                            {
                                auto pRenderableBezier = dynamic_cast<IBezier*>(pRenderable.get());

                                if (pRenderableBezier != nullptr)
                                {
                                    pRenderableBezier->regenerateData();
                                }
                            }
                        }
                        else if (auto pSelectedBezierC2 = dynamic_cast<BezierC2*>(pSelectedRenderable); pSelectedBezierC2 != nullptr)
                        {
                            // Selected bezier C2
                            // TODO: update only beziers containing modified control points
                            for (const Id controlPointRenderableId : pSelectedBezierC2->mControlPointIds)
                            {
                                IRenderable* const pControlPointRenderable =
                                    mpRenderer->getRenderable(controlPointRenderableId);

                                pControlPointRenderable->mWorldPos += offsetVec;
                            }

                            for (const auto& pRenderable : mpRenderer->mRenderables)
                            {
                                auto pRenderableBezier = dynamic_cast<IBezier*>(pRenderable.get());

                                if (pRenderableBezier != nullptr)
                                {
                                    pRenderableBezier->regenerateData();
                                }
                            }
                        }
                        else if (auto pSelectedInterpolatedBezierC2 = dynamic_cast<InterpolatedBezierC2*>(pSelectedRenderable); pSelectedInterpolatedBezierC2 != nullptr)
                        {
                            // Selected interpolated bezier C2
                            // TODO: update only beziers containing modified control points
                            for (const Id controlPointRenderableId : pSelectedInterpolatedBezierC2->mControlPointIds)
                            {
                                IRenderable* const pControlPointRenderable =
                                    mpRenderer->getRenderable(controlPointRenderableId);

                                pControlPointRenderable->mWorldPos += offsetVec;
                            }

                            for (const auto& pRenderable : mpRenderer->mRenderables)
                            {
                                auto pRenderableBezier = dynamic_cast<IBezier*>(pRenderable.get());

                                if (pRenderableBezier != nullptr)
                                {
                                    pRenderableBezier->regenerateData();
                                }
                            }
                        }
                        else if (auto pSelectedBezierPatchC0 = dynamic_cast<BezierPatchC0*>(pSelectedRenderable); pSelectedBezierPatchC0 != nullptr)
                        {
                            for (const Id controlPointId : pSelectedBezierPatchC0->mControlPointIds)
                            {
                                IRenderable* const pControlPointRenderable =
                                    mpRenderer->getRenderable(controlPointId);

                                pControlPointRenderable->mWorldPos += offsetVec;
                            }

                            pSelectedBezierPatchC0->regenerateData();
                        }
                        else
                        {
                            pSelectedRenderable->mWorldPos += offsetVec;
                        }
                    }
                    break;
                    case InteractionType::SCALE:
                        pSelectedRenderable->mScale += -yOffset * 0.1f;
                        break;
                    }
                }
            }
        }
    }
    break;
    case IWindow::Message::KEY_DELETE_DOWN:
        for (auto it = mCtx.selectedRenderableIds.begin(); it != mCtx.selectedRenderableIds.end();)
        {
            if (!mpRenderer->getRenderable(*it)->isDeletable())
            {
                mMessage.set(Message::UNABLE_TO_DELETE);
                ++it;
                continue;
            }

            pDX11Renderer->removeRenderable(*it);
            mCtx.selectedRenderableIds.erase(*it);
            it = mCtx.selectedRenderableIds.begin();
        }

        // TODO: investigate it
        for (auto& pRenderable : pDX11Renderer->mRenderables)
        {
            pRenderable->regenerateData();
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
    }

    if (ImGui::IsWindowHovered())
    {
        mCtx.isMenuHovered = true;
    }
    else
    {
        mCtx.isMenuHovered = false;
    }

    bool isBernsteinEdited = mSelectedBernsteinPointIdx != -1;

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
        IBezier* pBezier = nullptr;
        if (mCtx.selectedRenderableIds.size() == 1)
        {
            const Id selectedRenderableId = *mCtx.selectedRenderableIds.begin();
            IRenderable* const pRenderable = mpRenderer->getRenderable(selectedRenderableId);
            pBezier = dynamic_cast<IBezier*>(pRenderable);
        }

        auto pPoint = std::make_unique<Point>(mCtx.cursorPos);
        pPoint->regenerateData();
        const Id pointId = pPoint->id;
        mpRenderer->addRenderable(std::move(pPoint));

        if (pBezier->id != invalidId)
        {
            pBezier->insertDeBoorPointId(pointId);
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
            mBezierPatchCreator = std::make_optional<BezierPatchCreator>();
        }

        ImGui::InputInt("U patches", &mBezierPatchCreator->u);
        ImGui::InputInt("V patches", &mBezierPatchCreator->v);

        ImGui::Checkbox("Wrapped", &mBezierPatchCreator->isWrapped);
        ImGui::Checkbox("C2", &mBezierPatchCreator->isC2);

        if (ImGui::Button("Create"))
        {
            auto pBezierPatchC0 = std::make_unique<BezierPatchC0>(std::move(*mBezierPatchCreator), mCtx.cursorPos, mpRenderer);
            pBezierPatchC0->regenerateData();
            mpRenderer->addRenderable(std::move(pBezierPatchC0));

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

    ImGui::Spacing();

    if (ImGui::Button("Clear Demo"))
    {
        mpRenderer->mRenderables.clear();
        mCtx.selectedRenderableIds.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Unselect All"))
    {
        mCtx.selectedRenderableIds.clear();
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

    int newSelectedItemIdx = -1;
    for (int i = 0; i < mpRenderer->mRenderables.size(); ++i)
    {
        const auto& pRenderable = mpRenderer->mRenderables.at(i);

        if (ImGui::Selectable(pRenderable->mTag.c_str(), mCtx.selectedRenderableIds.contains(pRenderable->id)))
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

            if (mCtx.selectedRenderableIds.contains(pRenderable->id))
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

        // TODO: add unselection with shift pressed
        if (mCtx.isShiftPressed && (mCtx.lastSelectedRenderableId != invalidId))
        {
            auto it = std::ranges::find_if(mpRenderer->mRenderables, [this](const auto& pRenderable) {
                return pRenderable->id == mCtx.lastSelectedRenderableId;
            });

            if (it != mpRenderer->mRenderables.end())
            {
                const size_t startIdx = std::distance(mpRenderer->mRenderables.begin(), it);
                const size_t endIdx = newSelectedItemIdx;

                const size_t minIdx = std::min(startIdx, endIdx);
                const size_t maxIdx = std::max(startIdx, endIdx);

                for (size_t idx = minIdx; idx <= maxIdx; ++idx)
                {
                    const Id id = mpRenderer->mRenderables.at(idx)->id;
                    mCtx.selectedRenderableIds.insert(id);
                    mSelectedBernsteinPointIdx = -1;
                }
            }
        }
        else
        {
            const auto newId = mpRenderer->mRenderables.at(newSelectedItemIdx)->id;
            if (!mCtx.selectedRenderableIds.contains(newId))
            {
                if (!mCtx.isCtrlClicked)
                {
                    mCtx.selectedRenderableIds.clear();
                }

                mCtx.selectedRenderableIds.insert(newId);
                mCtx.lastSelectedRenderableId = newId;
                mSelectedBernsteinPointIdx = -1;
            }
            else
            {
                mCtx.selectedRenderableIds.erase(newId);
                mSelectedBernsteinPointIdx = -1;
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
        const Id selectedRenderableId = *mCtx.selectedRenderableIds.begin();

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

        // TODO: add debug window
#ifndef NDEBUG
        if (ImGui::TreeNode("DEBUG"))
        {
            ImGui::Text("ENTITY ID: %d", pSelectedRenderable->id);
            ImGui::TreePop();
        }
#endif

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

        if (pSelectedRenderable->isScalable())
        {
            ImGui::BeginDisabled();
        }

        ImGui::Text("Scale:");
        ImGui::DragFloat("##scale", &pSelectedRenderable->mScale);
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
        else if (auto pBezier = dynamic_cast<IBezier*>(pSelectedRenderable); pBezier != nullptr)
        {
            if (dynamic_cast<InterpolatedBezierC2*>(pSelectedRenderable) == nullptr)
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
                    pBezier->mControlPointIds.erase(std::ranges::remove(pBezier->mControlPointIds, idToDelete).begin(),
                                              pBezier->mControlPointIds.end());
                    pBezier->regenerateData();
                }

                if (idToEdit != invalidId)
                {
                    mCtx.selectedRenderableIds.clear();
                    mCtx.selectedRenderableIds.insert(idToEdit);
                }

                ImGui::TreePop();
            }

            if (auto pBezierC2 = dynamic_cast<BezierC2*>(pBezier); pBezierC2 != nullptr)
            {
                if (ImGui::TreeNode("Bernstein points"))
                {
                    size_t idx = 0;
                    for (XMFLOAT3& bernsteinPoint : pBezierC2->getBernsteinPositions())
                    {
                        bool isThisBernsteinPointSelected = mSelectedBernsteinPointIdx == idx;
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
                                mSelectedBernsteinPointIdx = idx;
                            }
                            else
                            {
                                mSelectedBernsteinPointIdx = -1;
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
                        idx++;
                    }

                    ImGui::TreePop();
                }
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
        std::vector<Id> deBoorIds;

        for (const auto id : mCtx.selectedRenderableIds)
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
                auto pBezier = std::make_unique<BezierC0>(deBoorIds, mpRenderer);
                pBezier->regenerateData();
                mpRenderer->addRenderable(std::move(pBezier));
                isBezierAdded = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Bezier C2"))
            {
                auto pBezier = std::make_unique<BezierC2>(deBoorIds, mpRenderer);
                pBezier->regenerateData();
                mpRenderer->addRenderable(std::move(pBezier));
                isBezierAdded = true;
            }
            if (ImGui::Button("Add InterpolatedBezier C2")) // TODO: try to keep buttons in the same line
            {
                auto pBezier = std::make_unique<InterpolatedBezierC2>(deBoorIds, mpRenderer);
                pBezier->regenerateData();
                mpRenderer->addRenderable(std::move(pBezier));
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

    if (isBernsteinEdited)
    {
        ImGui::EndDisabled();
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

    long long int idx = 0;
    for (const XMFLOAT3& bernsteinPos : bernsteinPositions)
    {
        const XMVECTOR bernsteinVec = XMLoadFloat3(&bernsteinPos);
        pDX11Renderer->mGlobalCB.modelMtx =
            XMMatrixScaling(0.05f, 0.05f, 0.05f) * XMMatrixTranslationFromVector(bernsteinVec);
        if (idx == mSelectedBernsteinPointIdx)
        {
            pDX11Renderer->mGlobalCB.color = defaultSelectionColor;
        }
        else
        {
            pDX11Renderer->mGlobalCB.color = defaultColor;
        }
        pDX11Renderer->updateCB(pDX11Renderer->mGlobalCB);
        pDX11Renderer->getContext()->Draw(6, 0); // TODO: drawInstanced
        idx++;
    }
}
