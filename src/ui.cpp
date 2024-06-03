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

import core;
import app;
import window;
import dx11renderer;

using namespace std::chrono;
using namespace DirectX;
using namespace Microsoft::WRL;
namespace fs = std::filesystem;

std::optional<App::Demo> showMenu(App::Settings& settings, const char* demoName)
{
    std::optional<App::Demo> result{};

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Demo"))
        {
            if (ImGui::MenuItem("CAD", "F1"))
            {
                result = App::Demo::CAD;
            }
            else if (ImGui::MenuItem("Duck", "F2"))
            {
                result = App::Demo::DUCK;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings"))
        {
            ImGui::Checkbox("VSync", &settings.isVsync);

            ImGui::EndMenu();
        }

        ImGui::Text("| Current demo: %s", demoName);

        ImGui::EndMainMenuBar();
    }

    return result;
}

void App::renderUi()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    std::optional<App::Demo> selectedMode = showMenu(mSettings, mpDemo->mpDemoName);
    if (selectedMode != std::nullopt)
    {
        loadDemo(*selectedMode);
    }

    mIsMenuEnabled = false;
    mIsUiClicked = false;

    const float menuWidth = mpWindow->getWidth() * 0.2f;

    ImGui::SetNextWindowPos(ImVec2{mpWindow->getWidth() - menuWidth, 0});
    ImGui::SetNextWindowSize(ImVec2{menuWidth, static_cast<float>(mpWindow->getHeight())});

    if (ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
    {
        mIsMenuEnabled = true;
    }

    if (ImGui::IsWindowHovered())
    {
        mIsMenuHovered = true;
    }
    else
    {
        mIsMenuHovered = false;
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
        IBezier* pBezier = nullptr;
        if (mSelectedRenderableIds.size() == 1)
        {
            const IRenderable::Id selectedRenderableId = *mSelectedRenderableIds.begin();
            IRenderable* const pRenderable = mpRenderer->getRenderable(selectedRenderableId);
            pBezier = dynamic_cast<IBezier*>(pRenderable);
        }

        auto pPoint = std::make_unique<Point>(mCursorPos);
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

        if (ImGui::Selectable(pRenderable->mTag.c_str(), mSelectedRenderableIds.contains(pRenderable->mId)))
        {
            newSelectedItemIdx = i;
            mIsUiClicked |= ImGui::IsItemActive();
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    if (ImGui::IsItemHovered())
    {
        mIsMenuHovered = true;
    }

    if (newSelectedItemIdx != -1)
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

        if (mIsShiftPressed && (mLastSelectedRenderableId != IRenderable::invalidId))
        {
            auto it = std::ranges::find_if(mpRenderer->mRenderables, [this](const auto& pRenderable) {
                return pRenderable->mId == mLastSelectedRenderableId;
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
                    mSelectedRenderableIds.insert(id);
                }
            }
        }
        else
        {
            const auto newId = mpRenderer->mRenderables.at(newSelectedItemIdx)->mId;
            if (!mSelectedRenderableIds.contains(newId))
            {
                if (!mIsCtrlClicked)
                {
                    mSelectedRenderableIds.clear();
                }

                mSelectedRenderableIds.insert(newId);
                mLastSelectedRenderableId = newId;
            }
            else
            {
                mSelectedRenderableIds.erase(newId);
            }
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

        if (newSelectedItemIdx != -1)
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
        if (ImGui::DragFloat3("##localPos", localPos))
        {
            pSelectedRenderable->mLocalPos = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(localPos));
            pSelectedRenderable->mWorldPos = pSelectedRenderable->mLocalPos - mCursorPos;
        }
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Text("World Position:");
        if (ImGui::DragFloat3("##worldPos", worldPos))
        {
            pSelectedRenderable->mWorldPos = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(worldPos));
            pSelectedRenderable->mLocalPos = pSelectedRenderable->mWorldPos - mCursorPos;
        }
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Rotation:");
        if (ImGui::DragFloat3("##rotation", rotation))
        {
            pSelectedRenderable->mPitch = rotation[0];
            pSelectedRenderable->mYaw = rotation[1];
            pSelectedRenderable->mRoll = rotation[2];
        }
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Scale:");
        ImGui::DragFloat("##scale", &pSelectedRenderable->mScale);
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
            // mIsUiClicked |= ImGui::IsItemActive();

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
                        mSelectedRenderableIds.clear();
                        mSelectedRenderableIds.insert(id);
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
            mpRenderer->buildGeometryBuffers();
        }

        mIsUiClicked |= dataChanged;
    }
    else if (mSelectedRenderableIds.size() > 1)
    {
        std::unordered_set<IRenderable::Id> selectedPointsIds;

        for (const auto mId : mSelectedRenderableIds)
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
                mSelectedRenderableIds.clear();
            }
        }

        float pivotPos[3]{};

        XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(pivotPos), mPivotPos);

        ImGui::Text("Pivot position:");
        if (ImGui::DragFloat3("##pivotPos", pivotPos))
        {
            mPivotPos = XMLoadFloat3(reinterpret_cast<XMFLOAT3*>(pivotPos));
        }
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Pivot Pitch:");
        ImGui::DragFloat("##pivotPitch", &mPivotPitch);
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Pivot Yaw:");
        ImGui::DragFloat("##pivotYaw", &mPivotYaw);
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Pivot Roll:");
        ImGui::DragFloat("##pivotRoll", &mPivotRoll);
        mIsUiClicked |= ImGui::IsItemActive();

        ImGui::Text("Pivot Scale:");
        ImGui::DragFloat("##pivotScale", &mPivotScale);
        mIsUiClicked |= ImGui::IsItemActive();
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}