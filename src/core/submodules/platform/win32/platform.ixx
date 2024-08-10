module;

#include "Windows.h"
#include <windowsx.h>

#include "utils.h"

#include "imgui_impl_win32.h"

#define LIBRARY_TYPE HMODULE
#define LoadFunction GetProcAddress

export module platform;
import std.core;
import window;

export class Win32Window final : public IWindow
{
    friend LRESULT staticWindowProcedure(HWND hwnd, UINT msg, WPARAM pWParam, LPARAM wLParam);

public:
    Win32Window(int width, int height) : IWindow{width, height}
    {
    }
    ~Win32Window();

    HINSTANCE getHInstance() const
    {
        return mHInstance;
    }

    HWND getHwnd() const
    {
        return mHwnd;
    }

    void show() override;
    Message getMessage() override;

private:
    static LRESULT CALLBACK staticWindowProcedure(HWND hwnd, UINT msg, WPARAM pWParam, LPARAM wLParam);
    bool instanceWindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HINSTANCE mHInstance{};
    HWND mHwnd{};

    void init() override;
};

module :private;

void Win32Window::init()
{
    const WNDCLASSEX wc{
        .cbSize = sizeof(WNDCLASSEX),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = staticWindowProcedure,
        .hInstance = mHInstance,
        .hCursor = LoadCursor(nullptr, IDC_ARROW),
        .hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
        .lpszClassName = PROJECT_NAME,
    };

    if (!RegisterClassEx(&wc))
    {
        ERR("WNDCLASSEX registration failure");
    }

    const int currentScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int currentScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    const int screenCenterX = (currentScreenWidth - mWidth) / 2;
    const int screenCenterY = (currentScreenHeight - mHeight) / 2;
    mHwnd = CreateWindow(PROJECT_NAME, PROJECT_NAME, WS_OVERLAPPEDWINDOW, screenCenterX, screenCenterY,
                         static_cast<int>(mWidth), static_cast<int>(mHeight), nullptr, nullptr, mHInstance, this);

    if (mHwnd == nullptr)
    {
        ERR("Window creation failure");
    }

    ImGui_ImplWin32_Init(mHwnd);
}

Win32Window::~Win32Window()
{
    ImGui_ImplWin32_Shutdown();

    if (mHwnd != nullptr)
    {
        DestroyWindow(mHwnd);
    }

    if (mHInstance != nullptr)
    {
        UnregisterClass(PROJECT_NAME, mHInstance);
    }
}

void Win32Window::show()
{
    ShowWindow(mHwnd, SW_SHOWDEFAULT);
    UpdateWindow(mHwnd);
}

IWindow::Message Win32Window::getMessage()
{
    Message msg = IWindow::getMessage();
    if (msg == Message::EMPTY)
    {
        MSG winMsg;

        const bool isEmpty = PeekMessage(&winMsg, nullptr, 0, 0, PM_REMOVE) == 0;
        if (isEmpty)
        {
            return Message::EMPTY;
        }
        else if (winMsg.message == WM_QUIT)
        {
            return Message::QUIT;
        }

        TranslateMessage(&winMsg);
        DispatchMessage(&winMsg);

        auto potentialMsg = static_cast<Message>(winMsg.message);
        if ((potentialMsg > Message::EMPTY) && (potentialMsg < Message::COUNT))
        {
            msg = static_cast<Message>(winMsg.message);
        }
        else
        {
            msg = Message::EMPTY;
        }
    }

    return msg;
}

bool Win32Window::instanceWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (!mIsProcessing)
    {
        return false;
    }
    
    // TODO: consider GetAsyncKeyState for checking pressed buttons
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return true;
    case WM_SIZE:
        PostMessage(hwnd, static_cast<UINT32>(Message::RESIZE), wParam, lParam);
        mWidth = LOWORD(lParam);
        mHeight = HIWORD(lParam);
        return true;
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_F1:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_F1), wParam, lParam);
            break;
        case VK_F2:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_F2), wParam, lParam);
            break;
        case VK_F3:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_F3), wParam, lParam);
            break;
        case VK_F4:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_F4), wParam, lParam);
            break;
        case VK_F5:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_F5), wParam, lParam);
            break;
        case VK_F6:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_F6), wParam, lParam);
            break;
        case VK_F7:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_F7), wParam, lParam);
            break;
        case VK_F8:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_F8), wParam, lParam);
            break;
        case VK_F9:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_F9), wParam, lParam);
            break;
        case VK_F10:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_F10), wParam, lParam);
            break;
        case VK_F11:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_F11), wParam, lParam);
            break;
        case VK_F12:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_F12), wParam, lParam);
            break;
        case 'E':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_E_DOWN), wParam, lParam);
            return true;
        case 'W':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_W_DOWN), wParam, lParam);
            return true;
        case 'S':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_S_DOWN), wParam, lParam);
            return true;
        case 'A':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_A_DOWN), wParam, lParam);
            return true;
        case 'D':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_D_DOWN), wParam, lParam);
            return true;
        case VK_DELETE:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_DELETE_DOWN), wParam, lParam);
            return true;
        case VK_CONTROL:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_CTRL_DOWN), wParam, lParam);
            return true;
        case VK_SHIFT:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_SHIFT_DOWN), wParam, lParam);
            return true;
        }
        break;
    case WM_KEYUP:
        switch (wParam)
        {
        case 'E':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_E_UP), wParam, lParam);
            return true;
        case 'W':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_W_UP), wParam, lParam);
            return true;
        case 'S':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_S_UP), wParam, lParam);
            return true;
        case 'A':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_A_UP), wParam, lParam);
            return true;
        case 'D':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_D_UP), wParam, lParam);
            return true;
        case VK_DELETE:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_DELETE_UP), wParam, lParam);
            return true;
        case VK_CONTROL:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_CTRL_UP), wParam, lParam);
            return true;
        case VK_SHIFT:
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_SHIFT_UP), wParam, lParam);
            return true;
        }
        break;
    case WM_LBUTTONDOWN:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_LEFT_DOWN), wParam, lParam);
        goto setMousePositionLabel;
    case WM_LBUTTONUP:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_LEFT_UP), wParam, lParam);
        goto setMousePositionLabel;
    case WM_MBUTTONDOWN:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_MIDDLE_DOWN), wParam, lParam);
        goto setMousePositionLabel;
    case WM_MBUTTONUP:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_MIDDLE_UP), wParam, lParam);
        goto setMousePositionLabel;
    case WM_RBUTTONDOWN:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_RIGHT_DOWN), wParam, lParam);
        goto setMousePositionLabel;
    case WM_RBUTTONUP:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_RIGHT_UP), wParam, lParam);
        goto setMousePositionLabel;
    case WM_MOUSEMOVE:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_MOVE), wParam, lParam);
        goto setMousePositionLabel;
    case WM_MOUSEWHEEL: {
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_WHEEL), wParam, lParam);

        const Event::MouseWheel mouseWheel{
            .yOffset = GET_WHEEL_DELTA_WPARAM(wParam),
        };
        mEventData.push(mouseWheel);

        return true;
    }
    setMousePositionLabel:
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);

        const Event::MousePosition mousePosition{
            .x = x,
            .y = y,
        };

        mEventData.push(mousePosition);
        return true;
    }

    return false;
}

extern "C++" IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK Win32Window::staticWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
    {
        return true;
    }

    Win32Window* pThis{};
    if (msg == WM_NCCREATE)
    {
        LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
        pThis = static_cast<Win32Window*>(lpcs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if ((pThis != nullptr) && (!pThis->instanceWindowProcedure(hwnd, msg, wParam, lParam)))
    {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}