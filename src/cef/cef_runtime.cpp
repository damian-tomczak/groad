#include "cef/cef_runtime.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <mutex>

#ifdef GROAD_WITH_CEF
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_client.h"
#endif

namespace groad::cef
{
namespace
{
std::filesystem::path getExecutablePath()
{
    std::array<wchar_t, MAX_PATH> buffer{};
    const DWORD pathLength = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    return std::filesystem::path(std::wstring_view(buffer.data(), pathLength));
}

std::string makeFileUrl(const std::filesystem::path& path)
{
    return "file:///" + std::filesystem::absolute(path).generic_string();
}
}

#ifdef GROAD_WITH_CEF
namespace
{
class GroadCefApp final : public CefApp
{
public:
    IMPLEMENT_REFCOUNTING(GroadCefApp);
};

bool gIsInitialized = false;
CefRefPtr<GroadCefApp> gApp;

int getMouseModifiers(WPARAM wParam)
{
    int modifiers = EVENTFLAG_NONE;

    if (((wParam & MK_CONTROL) != 0) || (GetKeyState(VK_CONTROL) < 0))
    {
        modifiers |= EVENTFLAG_CONTROL_DOWN;
    }
    if (((wParam & MK_SHIFT) != 0) || (GetKeyState(VK_SHIFT) < 0))
    {
        modifiers |= EVENTFLAG_SHIFT_DOWN;
    }
    if (GetKeyState(VK_MENU) < 0)
    {
        modifiers |= EVENTFLAG_ALT_DOWN;
    }
    if ((wParam & MK_LBUTTON) != 0)
    {
        modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
    }
    if ((wParam & MK_MBUTTON) != 0)
    {
        modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
    }
    if ((wParam & MK_RBUTTON) != 0)
    {
        modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
    }

    return modifiers;
}

int getKeyboardModifiers(WPARAM wParam, LPARAM lParam)
{
    int modifiers = EVENTFLAG_NONE;

    if (GetKeyState(VK_SHIFT) < 0)
    {
        modifiers |= EVENTFLAG_SHIFT_DOWN;
    }
    if (GetKeyState(VK_CONTROL) < 0)
    {
        modifiers |= EVENTFLAG_CONTROL_DOWN;
    }
    if (GetKeyState(VK_MENU) < 0)
    {
        modifiers |= EVENTFLAG_ALT_DOWN;
    }
    if (GetKeyState(VK_NUMLOCK) & 1)
    {
        modifiers |= EVENTFLAG_NUM_LOCK_ON;
    }
    if (GetKeyState(VK_CAPITAL) & 1)
    {
        modifiers |= EVENTFLAG_CAPS_LOCK_ON;
    }

    switch (wParam)
    {
    case VK_RETURN:
        if ((lParam & 0x01000000) != 0)
        {
            modifiers |= EVENTFLAG_IS_KEY_PAD;
        }
        break;
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_NUMLOCK:
        if ((lParam & 0x01000000) == 0)
        {
            modifiers |= EVENTFLAG_IS_KEY_PAD;
        }
        break;
    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9:
    case VK_DIVIDE:
    case VK_MULTIPLY:
    case VK_SUBTRACT:
    case VK_ADD:
    case VK_DECIMAL:
    case VK_CLEAR:
        modifiers |= EVENTFLAG_IS_KEY_PAD;
        break;
    case VK_LSHIFT:
    case VK_LCONTROL:
    case VK_LMENU:
        modifiers |= EVENTFLAG_IS_LEFT;
        break;
    case VK_RSHIFT:
    case VK_RCONTROL:
    case VK_RMENU:
        modifiers |= EVENTFLAG_IS_RIGHT;
        break;
    }

    return modifiers;
}

int getXLParam(LPARAM lParam)
{
    return static_cast<int>(static_cast<short>(LOWORD(lParam)));
}

int getYLParam(LPARAM lParam)
{
    return static_cast<int>(static_cast<short>(HIWORD(lParam)));
}

int getWheelDelta(WPARAM wParam)
{
    return static_cast<int>(static_cast<short>(HIWORD(wParam)));
}

WPARAM getKeyStateWParam(WPARAM wParam)
{
    return LOWORD(wParam);
}
}

class BrowserClient final : public CefClient,
                            public CefDisplayHandler,
                            public CefLifeSpanHandler,
                            public CefRenderHandler
{
public:
    explicit BrowserClient(BrowserSurface::Impl* pOwner) : mpOwner(pOwner)
    {
    }

    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override
    {
        return this;
    }

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
    {
        return this;
    }

    CefRefPtr<CefRenderHandler> GetRenderHandler() override
    {
        return this;
    }

    void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    void OnPaint(CefRefPtr<CefBrowser> browser,
                 PaintElementType type,
                 const RectList& dirtyRects,
                 const void* buffer,
                 int width,
                 int height) override;

private:
    BrowserSurface::Impl* mpOwner;

    IMPLEMENT_REFCOUNTING(BrowserClient);
};

struct BrowserSurface::Impl
{
    std::mutex mutex;
    RECT rect{0, 0, 1, 1};
    HWND parentWindow = nullptr;
    bool focused = false;
    bool mouseInside = false;
    bool trackingMouseLeave = false;
    bool dirty = false;
    bool closeRequested = false;
    bool closeParentOnBeforeClose = false;
    std::wstring title = L"CEF Browser";
    std::vector<std::uint8_t> pixels;

    CefRefPtr<BrowserClient> client;
    CefRefPtr<CefBrowser> browser;

    int width = 1;
    int height = 1;
};

void BrowserClient::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    std::scoped_lock lock(mpOwner->mutex);
    mpOwner->title = title.ToWString();
}

void BrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    mpOwner->browser = browser;
    mpOwner->closeRequested = false;
    mpOwner->closeParentOnBeforeClose = false;
    browser->GetHost()->WasResized();
}

void BrowserClient::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    mpOwner->browser = nullptr;
    mpOwner->focused = false;
    mpOwner->mouseInside = false;
    mpOwner->trackingMouseLeave = false;

    const bool shouldCloseParentWindow = mpOwner->closeParentOnBeforeClose;
    const HWND parentWindow = mpOwner->parentWindow;

    mpOwner->closeRequested = false;
    mpOwner->closeParentOnBeforeClose = false;

    if (shouldCloseParentWindow && (parentWindow != nullptr) && IsWindow(parentWindow))
    {
        PostMessage(parentWindow, WM_CLOSE, 0, 0);
    }
}

void BrowserClient::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    const auto width = static_cast<int>(std::max<LONG>(1, mpOwner->rect.right - mpOwner->rect.left));
    const auto height = static_cast<int>(std::max<LONG>(1, mpOwner->rect.bottom - mpOwner->rect.top));

    rect = CefRect(0, 0, width, height);
}

void BrowserClient::OnPaint(CefRefPtr<CefBrowser> browser,
                            PaintElementType type,
                            const RectList& dirtyRects,
                            const void* buffer,
                            int width,
                            int height)
{
    if (type != PET_VIEW)
    {
        return;
    }

    std::scoped_lock lock(mpOwner->mutex);
    mpOwner->width = width;
    mpOwner->height = height;
    mpOwner->dirty = true;

    const size_t byteCount = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    mpOwner->pixels.resize(byteCount);
    std::memcpy(mpOwner->pixels.data(), buffer, byteCount);
}
#endif

StartupResult initialize(HINSTANCE hInstance)
{
#ifdef GROAD_WITH_CEF
    if (gIsInitialized)
    {
        return {};
    }

    CefMainArgs mainArgs(hInstance);
    gApp = new GroadCefApp();

    const int exitCode = CefExecuteProcess(mainArgs, gApp.get(), nullptr);
    if (exitCode >= 0)
    {
        return {
            .shouldExit = true,
            .exitCode = exitCode,
        };
    }

    CefSettings settings;
    settings.no_sandbox = true;
    settings.windowless_rendering_enabled = true;
    settings.external_message_pump = false;
    settings.multi_threaded_message_loop = false;

    const auto executablePath = getExecutablePath();
    const auto executableDir = executablePath.parent_path();

    CefString(&settings.browser_subprocess_path) = executablePath.wstring();
    CefString(&settings.resources_dir_path) = executableDir.wstring();
    CefString(&settings.locales_dir_path) = (executableDir / "locales").wstring();

    if (!CefInitialize(mainArgs, settings, gApp.get(), nullptr))
    {
        return {
            .shouldExit = true,
            .exitCode = 1,
        };
    }

    gIsInitialized = true;
#else
    (void)hInstance;
#endif

    return {};
}

void shutdown()
{
#ifdef GROAD_WITH_CEF
    if (!gIsInitialized)
    {
        return;
    }

    CefShutdown();
    gApp = nullptr;
    gIsInitialized = false;
#endif
}

void doMessageLoopWork()
{
#ifdef GROAD_WITH_CEF
    if (gIsInitialized)
    {
        CefDoMessageLoopWork();
    }
#endif
}

bool isInitialized()
{
#ifdef GROAD_WITH_CEF
    return gIsInitialized;
#else
    return false;
#endif
}

BrowserSurface::BrowserSurface() : mImpl{std::make_unique<Impl>()}
{
}

BrowserSurface::~BrowserSurface()
{
    close();
}

bool BrowserSurface::create(HWND parentWindow, const std::filesystem::path& htmlPath)
{
#ifdef GROAD_WITH_CEF
    if (!isInitialized() || (parentWindow == nullptr))
    {
        return false;
    }

    if (mImpl->browser != nullptr)
    {
        return true;
    }

    mImpl->client = new BrowserClient(mImpl.get());
    mImpl->parentWindow = parentWindow;
    mImpl->closeRequested = false;
    mImpl->closeParentOnBeforeClose = false;

    CefWindowInfo windowInfo;
    windowInfo.SetAsWindowless(parentWindow);

    CefBrowserSettings browserSettings;
    browserSettings.windowless_frame_rate = 60;
    browserSettings.background_color = CefColorSetARGB(0, 0, 0, 0);

    mImpl->browser = CefBrowserHost::CreateBrowserSync(
        windowInfo,
        mImpl->client.get(),
        makeFileUrl(htmlPath),
        browserSettings,
        nullptr,
        nullptr
    );

    return mImpl->browser != nullptr;
#else
    (void)parentWindow;
    (void)htmlPath;
    return false;
#endif
}

void BrowserSurface::close()
{
#ifdef GROAD_WITH_CEF
    if (mImpl == nullptr)
    {
        return;
    }

    mImpl->closeRequested = true;
    mImpl->closeParentOnBeforeClose = false;

    if (mImpl->browser != nullptr)
    {
        mImpl->browser->GetHost()->CloseBrowser(true);

        for (int i = 0; (i < 60) && (mImpl->browser != nullptr); ++i)
        {
            CefDoMessageLoopWork();
            Sleep(10);
        }
    }

    mImpl->client = nullptr;
#endif
}

void BrowserSurface::setRect(int left, int top, int width, int height)
{
    width = std::max(1, width);
    height = std::max(1, height);

    const bool sizeChanged = ((mImpl->rect.right - mImpl->rect.left) != width) || ((mImpl->rect.bottom - mImpl->rect.top) != height);

    mImpl->rect.left = left;
    mImpl->rect.top = top;
    mImpl->rect.right = left + width;
    mImpl->rect.bottom = top + height;

#ifdef GROAD_WITH_CEF
    if (sizeChanged && (mImpl->browser != nullptr))
    {
        mImpl->browser->GetHost()->WasResized();
    }
#endif
}

void BrowserSurface::reload()
{
#ifdef GROAD_WITH_CEF
    if (mImpl->browser != nullptr)
    {
        mImpl->browser->Reload();
    }
#endif
}

bool BrowserSurface::handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#ifdef GROAD_WITH_CEF
    CefRefPtr<CefBrowser> browser = mImpl->browser;
    if (browser == nullptr)
    {
        return false;
    }

    CefRefPtr<CefBrowserHost> host = browser->GetHost();
    if (host == nullptr)
    {
        return false;
    }

    const auto containsPoint = [this](POINT point) -> bool {
        return (point.x >= mImpl->rect.left) && (point.x < mImpl->rect.right)
            && (point.y >= mImpl->rect.top) && (point.y < mImpl->rect.bottom);
    };

    const auto toLocalPoint = [this](POINT point) -> CefMouseEvent {
        CefMouseEvent event{};
        event.x = point.x - mImpl->rect.left;
        event.y = point.y - mImpl->rect.top;
        return event;
    };

    switch (msg)
    {
    case WM_CLOSE:
        if (mImpl->closeRequested)
        {
            return true;
        }

        mImpl->closeRequested = true;
        mImpl->closeParentOnBeforeClose = true;
        host->CloseBrowser(false);
        return true;
    case WM_MOUSEMOVE:
    {
        POINT point{getXLParam(lParam), getYLParam(lParam)};
        const bool isInside = containsPoint(point);

        if (isInside)
        {
            if (!mImpl->trackingMouseLeave)
            {
                TRACKMOUSEEVENT event{
                    .cbSize = sizeof(TRACKMOUSEEVENT),
                    .dwFlags = TME_LEAVE,
                    .hwndTrack = hwnd,
                };
                TrackMouseEvent(&event);
                mImpl->trackingMouseLeave = true;
            }

            auto mouseEvent = toLocalPoint(point);
            mouseEvent.modifiers = getMouseModifiers(wParam);
            host->SendMouseMoveEvent(mouseEvent, false);
            mImpl->mouseInside = true;
            return true;
        }

        if (mImpl->mouseInside)
        {
            auto mouseEvent = toLocalPoint(point);
            mouseEvent.modifiers = getMouseModifiers(wParam);
            host->SendMouseMoveEvent(mouseEvent, true);
            mImpl->mouseInside = false;
        }

        return false;
    }
    case WM_MOUSELEAVE:
        mImpl->trackingMouseLeave = false;
        if (mImpl->mouseInside)
        {
            CefMouseEvent mouseEvent{};
            mouseEvent.x = static_cast<int>(std::max<LONG>(0, mImpl->rect.right - mImpl->rect.left - 1));
            mouseEvent.y = static_cast<int>(std::max<LONG>(0, mImpl->rect.bottom - mImpl->rect.top - 1));
            host->SendMouseMoveEvent(mouseEvent, true);
            mImpl->mouseInside = false;
            return mImpl->focused;
        }
        return false;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    {
        POINT point{getXLParam(lParam), getYLParam(lParam)};
        const bool isInside = containsPoint(point);

        if (!isInside)
        {
            if (mImpl->focused)
            {
                host->SetFocus(false);
                mImpl->focused = false;
            }
            return false;
        }

        if (!mImpl->focused)
        {
            host->SetFocus(true);
            mImpl->focused = true;
            ::SetFocus(hwnd);
        }

        auto mouseEvent = toLocalPoint(point);
        mouseEvent.modifiers = getMouseModifiers(wParam);

        CefBrowserHost::MouseButtonType buttonType = MBT_LEFT;
        if ((msg == WM_MBUTTONDOWN) || (msg == WM_MBUTTONUP))
        {
            buttonType = MBT_MIDDLE;
        }
        else if ((msg == WM_RBUTTONDOWN) || (msg == WM_RBUTTONUP))
        {
            buttonType = MBT_RIGHT;
        }

        const bool mouseUp = (msg == WM_LBUTTONUP) || (msg == WM_MBUTTONUP) || (msg == WM_RBUTTONUP);
        host->SendMouseClickEvent(mouseEvent, buttonType, mouseUp, 1);
        return true;
    }
    case WM_MOUSEWHEEL:
    {
        POINT point{getXLParam(lParam), getYLParam(lParam)};
        ScreenToClient(hwnd, &point);

        if (!containsPoint(point))
        {
            return false;
        }

        auto mouseEvent = toLocalPoint(point);
        mouseEvent.modifiers = getMouseModifiers(getKeyStateWParam(wParam));
        host->SendMouseWheelEvent(mouseEvent, 0, getWheelDelta(wParam));
        return true;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
    case WM_CHAR:
    case WM_SYSCHAR:
    {
        if ((wParam >= VK_F1) && (wParam <= VK_F12))
        {
            return false;
        }

        if (!mImpl->focused)
        {
            return false;
        }

        CefKeyEvent keyEvent{};
        keyEvent.windows_key_code = static_cast<int>(wParam);
        keyEvent.native_key_code = static_cast<int>(lParam);
        keyEvent.is_system_key = (msg == WM_SYSKEYDOWN) || (msg == WM_SYSKEYUP) || (msg == WM_SYSCHAR);
        keyEvent.modifiers = getKeyboardModifiers(wParam, lParam);

        if ((msg == WM_KEYDOWN) || (msg == WM_SYSKEYDOWN))
        {
            keyEvent.type = KEYEVENT_RAWKEYDOWN;
        }
        else if ((msg == WM_KEYUP) || (msg == WM_SYSKEYUP))
        {
            keyEvent.type = KEYEVENT_KEYUP;
        }
        else
        {
            keyEvent.type = KEYEVENT_CHAR;
            keyEvent.character = static_cast<char16_t>(wParam);
            keyEvent.unmodified_character = static_cast<char16_t>(wParam);
        }

        host->SendKeyEvent(keyEvent);
        return true;
    }
    case WM_KILLFOCUS:
        if (mImpl->focused)
        {
            host->SetFocus(false);
            mImpl->focused = false;
        }
        return false;
    default:
        return false;
    }
#else
    (void)hwnd;
    (void)msg;
    (void)wParam;
    (void)lParam;
    return false;
#endif
}

bool BrowserSurface::consumeFrame(std::vector<std::uint8_t>& pixels, int& width, int& height)
{
#ifdef GROAD_WITH_CEF
    if (mImpl == nullptr)
    {
        return false;
    }

    std::scoped_lock lock(mImpl->mutex);
    if (!mImpl->dirty)
    {
        return false;
    }

    pixels = mImpl->pixels;
    width = mImpl->width;
    height = mImpl->height;
    mImpl->dirty = false;
    return !pixels.empty();
#else
    (void)pixels;
    (void)width;
    (void)height;
    return false;
#endif
}

bool BrowserSurface::isReady() const
{
#ifdef GROAD_WITH_CEF
    return (mImpl != nullptr) && (mImpl->browser != nullptr);
#else
    return false;
#endif
}

std::wstring BrowserSurface::getTitle() const
{
#ifdef GROAD_WITH_CEF
    std::scoped_lock lock(mImpl->mutex);
    return mImpl->title;
#else
    return L"CEF unavailable";
#endif
}
}
