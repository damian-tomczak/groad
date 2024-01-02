module;

#include "Windows.h"
#include <windowsx.h>

#include "utils.hpp"

#define LIBRARY_TYPE HMODULE
#define LoadFunction GetProcAddress

export module platform;
import std.core;
import window;

export class Win32Window final : public IWindow
{
    friend LRESULT staticWindowProcedure(HWND hwnd, UINT msg, WPARAM pWParam, LPARAM wLParam);

public:
    Win32Window(const int width, const int height) : IWindow{width, height}
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
    Message peekMessage() override;
    void dispatchMessage() override;

private:
    static LRESULT CALLBACK staticWindowProcedure(HWND hwnd, UINT msg, WPARAM pWParam, LPARAM wLParam);
    LRESULT instanceWindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HINSTANCE mHInstance{};
    HWND mHwnd{};
    MSG mMsg{};

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
}

Win32Window::~Win32Window()
{
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

IWindow::Message Win32Window::peekMessage()
{
    PeekMessage(&mMsg, NULL, 0, 0, PM_REMOVE);

    return static_cast<Message>(mMsg.message);
}

void Win32Window::dispatchMessage()
{
    TranslateMessage(&mMsg);
    DispatchMessage(&mMsg);
}

LRESULT Win32Window::instanceWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
    case WM_EXITSIZEMOVE:
        PostMessage(hwnd, static_cast<UINT32>(Message::RESIZE), wParam, lParam);
        break;
    case WM_KEYDOWN:
        switch (wParam)
        {
        case 'W':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_W_DOWN), wParam, lParam);
            break;
        case 'S':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_S_DOWN), wParam, lParam);
            break;
        case 'A':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_A_DOWN), wParam, lParam);
            break;
        case 'D':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_D_DOWN), wParam, lParam);
            break;
        }
        break;
    case WM_KEYUP:
        switch (wParam)
        {
        case 'W':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_W_UP), wParam, lParam);
            break;
        case 'S':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_S_UP), wParam, lParam);
            break;
        case 'A':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_A_UP), wParam, lParam);
            break;
        case 'D':
            PostMessage(hwnd, static_cast<UINT32>(Message::KEY_D_UP), wParam, lParam);
            break;
        }
        break;
    case WM_CLOSE:
        PostMessage(hwnd, static_cast<UINT32>(Message::QUIT), wParam, lParam);
        break;
    case WM_LBUTTONDOWN:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_LEFT_DOWN), wParam, lParam);
        break;
    case WM_LBUTTONUP:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_LEFT_UP), wParam, lParam);
        break;
    case WM_RBUTTONDOWN:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_RIGHT_DOWN), wParam, lParam);
        break;
    case WM_RBUTTONUP:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_RIGHT_UP), wParam, lParam);
        break;
    case WM_MBUTTONDOWN:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_MIDDLE_DOWN), wParam, lParam);
        break;
    case WM_MBUTTONUP:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_MIDDLE_UP), wParam, lParam);
        break;
    case WM_MOUSEMOVE: {
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_MOVE), wParam, lParam);
        const EventData::MousePosition mousePosition{
            .xoffset = GET_X_LPARAM(lParam),
            .yoffset = GET_Y_LPARAM(lParam),
        };
        mEventData = mousePosition;
        break;
    }
    case WM_MOUSEWHEEL: {
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_WHEEL), wParam, lParam);
        const EventData::MousePosition mouseWheel{
            .yoffset = GET_WHEEL_DELTA_WPARAM(wParam),
        };
        mEventData = mouseWheel;
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

LRESULT CALLBACK Win32Window::staticWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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

    if (pThis)
    {
        return pThis->instanceWindowProcedure(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}