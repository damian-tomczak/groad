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
    Win32Window(unsigned width, unsigned height) : IWindow{width, height}
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
    if (PeekMessage(&mMsg, nullptr, 0, 0, PM_REMOVE) == 0)
    {
        return Message::EMPTY;
    }
    TranslateMessage(&mMsg);
    DispatchMessage(&mMsg);

    return static_cast<Message>(mMsg.message);
}

bool Win32Window::instanceWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return false;
    case WM_QUIT:
        PostMessage(hwnd, static_cast<UINT32>(Message::QUIT), wParam, lParam);
        return true;
    case WM_SIZE:
    case WM_EXITSIZEMOVE:
        PostMessage(hwnd, static_cast<UINT32>(Message::RESIZE), wParam, lParam);
        return true;
    case WM_KEYDOWN:
        switch (wParam)
        {
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
        }
        break;
    case WM_KEYUP:
        switch (wParam)
        {
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
        }
        break;
    case WM_LBUTTONDOWN:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_LEFT_DOWN), wParam, lParam);
        return true;
    case WM_LBUTTONUP:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_LEFT_UP), wParam, lParam);
        return true;
    case WM_RBUTTONDOWN:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_RIGHT_DOWN), wParam, lParam);
        return true;
    case WM_RBUTTONUP:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_RIGHT_UP), wParam, lParam);
        return true;
    case WM_MBUTTONDOWN:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_MIDDLE_DOWN), wParam, lParam);
        return true;
    case WM_MBUTTONUP:
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_MIDDLE_UP), wParam, lParam);
        return true;
    case WM_MOUSEMOVE: {
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_MOVE), wParam, lParam);

        if ((mLastXMousePosition != std::nullopt) && (mLastYMousePosition != std::nullopt))
        {
            const int currentXMousePos = GET_X_LPARAM(lParam);
            const int currentYMousePos = GET_Y_LPARAM(lParam);

            const int xoffset = currentXMousePos - *mLastXMousePosition;
            const int yoffset = currentYMousePos - *mLastYMousePosition;

            const Event::MousePosition mousePosition{
                .xoffset = xoffset,
                .yoffset = yoffset,
            };

            mLastXMousePosition = currentXMousePos;
            mLastYMousePosition = currentYMousePos;

            mEventData = mousePosition;
        }
        else
        {
            mLastXMousePosition = std::make_optional(GET_X_LPARAM(lParam));
            mLastYMousePosition = std::make_optional(GET_Y_LPARAM(lParam));
        }

        return true;
    }
    case WM_MOUSEWHEEL: {
        PostMessage(hwnd, static_cast<UINT32>(Message::MOUSE_WHEEL), wParam, lParam);
        const Event::MouseWheel mouseWheel{
            .yoffset = GET_WHEEL_DELTA_WPARAM(wParam),
        };
        mEventData = mouseWheel;
        return true;
    }
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