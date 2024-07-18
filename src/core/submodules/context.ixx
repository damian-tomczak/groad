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
import std.core;
import dx11renderer;
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

struct Context
{
    std::unordered_set<Id> selectedRenderableIds;
    Id lastSelectedRenderableId = invalidId;

    XMVECTOR pivotPos;
    float pivotPitch;
    float pivotYaw;
    float pivotRoll;
    float pivotScale;

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