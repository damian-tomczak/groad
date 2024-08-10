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

    long long int mSelectedBernsteinPointIdx = -1;

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
    };
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

    for (const auto [renderableIdx, pRenderable] : std::views::enumerate(pDX11Renderer->getRenderables()))
    {
        if (pRenderable == nullptr)
        {
            continue;
        }

        pDX11Renderer->mGlobalCB.color = pRenderable->mColor;

        XMMATRIX model = XMMatrixIdentity();

        XMMATRIX localScaleMat = XMMatrixScalingFromVector(pRenderable->mScale);
        XMMATRIX pivotScaleMat = XMMatrixIdentity();

        XMMATRIX pitchRotationMatrix = XMMatrixRotationX(pRenderable->mPitch);
        XMMATRIX yawRotationMatrix = XMMatrixRotationY(pRenderable->mYaw);
        XMMATRIX rollRotationMatrix = XMMatrixRotationZ(pRenderable->mRoll);

        XMMATRIX localRotationMat = pitchRotationMatrix * yawRotationMatrix * rollRotationMatrix;
        XMMATRIX pivotRotationMat = XMMatrixIdentity();

        XMMATRIX translationToOrigin = XMMatrixIdentity();
        XMMATRIX translationBack = XMMatrixIdentity();

        if (mCtx.selectedRenderableIds.contains(pRenderable->getId()))
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

        model = localScaleMat * localRotationMat *
                translationToOrigin * pivotScaleMat * pivotRotationMat * translationBack *
                worldTranslation;

        pDX11Renderer->mGlobalCB.modelMtx = model;

        if (mCtx.selectedRenderableIds.contains(pRenderable->getId()) ||
            pointIdsPartOfSelected.contains(pRenderable->getId()))
        {
            pDX11Renderer->mGlobalCB.color = defaultSelectionColor;
        }

        pDX11Renderer->updateCB(pDX11Renderer->mGlobalCB);

        ID3D11Buffer* const* const pVertexBuffer = pDX11Renderer->mVertexBuffers.at(renderableIdx).GetAddressOf();
        //ASSERT(*pVertexBuffer != nullptr);
        pDX11Renderer->getContext()->IASetVertexBuffers(0, 1, pVertexBuffer, &pRenderable->getStride(), &pRenderable->getOffset());

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
        if (auto pBezierC2 = dynamic_cast<BezierCurveC2*>(pRenderable); pBezierC2 != nullptr)
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
                    mCtx.selectedRenderableIds.insert(pRenderable->getId());
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

                if (auto pBezierC2 = dynamic_cast<BezierCurveC2*>(pSelectedRenderable); (pBezierC2 != nullptr) && (mSelectedBernsteinPointIdx != -1))
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
        for (auto& pRenderable : pDX11Renderer->getRenderables())
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
            IBezierCurve* pBezier = nullptr;
            if (mCtx.selectedRenderableIds.size() == 1)
            {
                const Id selectedRenderableId = *mCtx.selectedRenderableIds.begin();
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

            if (ImGui::Button("Create"))
            {
                std::unique_ptr<IRenderable> pRenderable;
                if (!mBezierPatchCreator->isC2)
                {
                    pRenderable =
                        std::make_unique<BezierSurfaceC0>(std::move(*mBezierPatchCreator), mpRenderer);
                }
                else
                {
                    pRenderable =
                        std::make_unique<BezierSurfaceC2>(std::move(*mBezierPatchCreator), mpRenderer);
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

        ImGui::Spacing();

        if (ImGui::Button("Clear Demo"))
        {
            mpRenderer->clearRenderables();
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
        for (int i = 0; i < mpRenderer->getRenderables().size(); ++i)
        {
            const auto& pRenderable = mpRenderer->getRenderables()[i];

            if (pRenderable == nullptr)
            {
                continue;
            }

            if (ImGui::Selectable(pRenderable->mTag.c_str(), mCtx.selectedRenderableIds.contains(pRenderable->getId())))
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

                XMMATRIX localScaleMat = XMMatrixScalingFromVector(pRenderable->mScale);
                XMMATRIX pivotScaleMat = XMMatrixIdentity();

                XMMATRIX pitchRotationMatrix = XMMatrixRotationX(pRenderable->mPitch);
                XMMATRIX yawRotationMatrix = XMMatrixRotationY(pRenderable->mYaw);
                XMMATRIX rollRotationMatrix = XMMatrixRotationZ(pRenderable->mRoll);

                XMMATRIX localRotationMat = pitchRotationMatrix * yawRotationMatrix * rollRotationMatrix;
                XMMATRIX pivotRotationMat = XMMatrixIdentity();

                XMMATRIX translationToOrigin = XMMatrixIdentity();
                XMMATRIX translationBack = XMMatrixIdentity();

                if (mCtx.selectedRenderableIds.contains(pRenderable->getId()))
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

                model = localScaleMat * localRotationMat *
                    translationToOrigin * pivotScaleMat * pivotRotationMat * translationBack *
                    worldTranslation;

                pRenderable->mWorldPos = model.r[3];

                const XMMATRIX scaleMat = localScaleMat * pivotScaleMat;
                pRenderable->mScale = XMVECTOR{XMVectorGetX(scaleMat.r[0]), XMVectorGetY(scaleMat.r[1]), XMVectorGetZ(scaleMat.r[2]), 1.f};

                const XMMATRIX rotationMat =
                    localRotationMat * translationToOrigin * pivotRotationMat * translationBack;
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
                auto it = std::ranges::find_if(mpRenderer->getRenderables(), [this](const auto& pRenderable) {
                    return pRenderable->getId() == mCtx.lastSelectedRenderableId;
                });

                if (it != mpRenderer->getRenderables().end())
                {
                    const size_t startIdx = std::distance(mpRenderer->getRenderables().begin(), it);
                    const size_t endIdx = newSelectedItemIdx;

                    const size_t minIdx = std::min(startIdx, endIdx);
                    const size_t maxIdx = std::max(startIdx, endIdx);

                    for (size_t idx = minIdx; idx <= maxIdx; ++idx)
                    {
                        const Id id = mpRenderer->getRenderables().at(idx)->getId();
                        mCtx.selectedRenderableIds.insert(id);
                        mSelectedBernsteinPointIdx = -1;
                    }
                }
            }
            else
            {
                const auto newId = mpRenderer->getRenderables().at(newSelectedItemIdx)->getId();
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
                        mCtx.selectedRenderableIds.clear();
                        mCtx.selectedRenderableIds.insert(idToEdit);
                    }

                    ImGui::TreePop();
                }

                if (auto pBezierC2 = dynamic_cast<BezierCurveC2*>(pBezier); pBezierC2 != nullptr)
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
                    auto pBezier = std::make_unique<BezierCurveC0>(deBoorIds, mpRenderer);
                    pBezier->regenerateData();
                    mpRenderer->addRenderable(std::move(pBezier));
                    isBezierAdded = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Add Bezier C2"))
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

// TODO: create abstraction
void openFileDialog(fs::path& path, std::function<void()> pResumeProcessing, std::function<void()> pStopProcessing)
{
    IFileOpenDialog* pFileOpen;
    HR(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog,
                            reinterpret_cast<void**>(&pFileOpen)));

    auto pConvertPathToWin = [](const wchar_t* pPath) -> std::wstring {
        std::wstring winPath{pPath};
        std::replace(winPath.begin(), winPath.end(), '/', '\\');
        return winPath;
    };

    IShellItem* pDefaultFolder;
    HR(SHCreateItemFromParsingName((pConvertPathToWin(W_ASSETS_PATH) + L"scenes"s).c_str(), NULL,
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
            .u = serialBezierSurfaceC0.patches[0].samples.x,
            .v = serialBezierSurfaceC0.patches[0].samples.y,
            .isWrapped = serialBezierSurfaceC0.uWrapped && serialBezierSurfaceC0.vWrapped,
            .isC2 = false,
        };

        auto pBezierSurfaceC0 = std::make_unique<BezierSurfaceC0>(std::move(bezierSurfaceCreator), mpRenderer);
        pBezierSurfaceC0->applySerializerData(serialBezierSurfaceC0);
        mpRenderer->addRenderable(std::move(pBezierSurfaceC0));
    }

    for (const MG1::BezierSurfaceC2& serialBezierSurfaceC2 : scene.surfacesC2)
    {
        ASSERT(!serialBezierSurfaceC2.patches.empty());

        const BezierSurfaceCreator bezierSurfaceCreator
        {
            .u = serialBezierSurfaceC2.patches[0].samples.x,
            .v = serialBezierSurfaceC2.patches[0].samples.y,
            .isWrapped = serialBezierSurfaceC2.uWrapped && serialBezierSurfaceC2.vWrapped,
            .isC2 = true,
        };

        auto pBezierSurfaceC2 = std::make_unique<BezierSurfaceC2>(std::move(bezierSurfaceCreator), mpRenderer);
        pBezierSurfaceC2->applySerializerData(serialBezierSurfaceC2);
        mpRenderer->addRenderable(std::move(pBezierSurfaceC2));
    }
}
