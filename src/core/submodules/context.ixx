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

export enum class InteractionType
{
    ROTATE,
    MOVE,
    SCALE,
    SELECT,
};

export inline const char* interationsNames[] = {"ROTATE", "MOVE", "SCALE", "SELECT"};

export struct Context
{
    std::unordered_set<IRenderable::Id> selectedRenderableIds;
    IRenderable::Id lastSelectedRenderableId = IRenderable::invalidId;

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
};