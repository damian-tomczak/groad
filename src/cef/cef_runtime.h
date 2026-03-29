#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace groad::cef
{
class BrowserClient;

struct StartupResult
{
    bool shouldExit = false;
    int exitCode = 0;
};

StartupResult initialize(HINSTANCE hInstance);
void shutdown();
void doMessageLoopWork();
bool isInitialized();
void setDisableGpuVsyncSwitch(bool disableGpuVsync);

class BrowserSurface final
{
public:
    using AcceleratedFrameCallback = std::function<void(HANDLE sharedTextureHandle, int width, int height)>;

    BrowserSurface();
    ~BrowserSurface();

    bool create(HWND parentWindow, const std::filesystem::path& htmlPath);
    void close();

    void setRect(int left, int top, int width, int height);
    void reload();
    void requestFrame();
    void setAcceleratedFrameCallback(AcceleratedFrameCallback callback);

    bool handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool consumeFrame(std::vector<std::uint8_t>& pixels, int& width, int& height);

    bool isReady() const;
    std::wstring getTitle() const;

private:
    friend class BrowserClient;

    struct Impl;
    std::unique_ptr<Impl> mImpl;
};
}
