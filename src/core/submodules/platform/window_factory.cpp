import utils;
import window;
import platform;

#include "utils.h"

IWindow* IWindow::createWindow(const uint32_t width, const uint32_t height)
{
    if (isWindowCreated)
    {
        ERR("Window is already created");
    }

    IWindow* pWindow;

#ifdef _WIN32
    pWindow = new Win32Window(width, height);
#else
#error "not implemented"
#endif

    isWindowCreated = true;

    return pWindow;
}