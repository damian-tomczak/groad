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

export module core.context;
import std;
import dxrenderer;
import window;

export
{
enum class InteractionType
{
    MOVE,
    ROTATE,
    SCALE,
    SELECT,
};

inline const char* interationsNames[] = {"MOVE", "ROTATE", "SCALE", "SELECT"};

struct RenderableSelection
{
    std::unordered_set<Id> renderableIds;

    template <typename RenderableType>
    std::vector<RenderableType*> getRenderables(DXRenderer* pRenderer)
    {
        std::vector<RenderableType*> result;
        for (const auto renderableId : renderableIds)
        {
            auto pRenderable = pRenderer->getRenderable(renderableId);
            if (auto pRenderableCasted = dynamic_cast<RenderableType*>(pRenderable))
            {
                result.push_back(pRenderableCasted);
            }
        }

        return result;
    }
};

struct Context
{
    struct RenderableSelection renderableSelection;
    Id lastSelectedRenderableId = invalidId;

    XMVECTOR centroidPos;
    float centroidPitch;
    float centroidYaw;
    float centroidRoll;
    float centroidScale;

    bool isMenuEnabled;
    bool isUiClicked;

    InteractionType interactionType;

    bool isMenuHovered;

    XMVECTOR cursorPos;
    bool isShiftPressed;
    bool isCtrlClicked;

    bool isLeftMouseClicked;
    bool isRightMouseClicked;

    float menuBarHeight;

    int lastXMousePosition = -1;
    int lastYMousePosition = -1;
};
}
