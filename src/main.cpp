import utils;
import core.options;
import app;

#include "utils.h"

struct HINSTANCE__;
using HINSTANCE = HINSTANCE__*;

extern "C" __declspec(dllimport) HINSTANCE __stdcall GetModuleHandleW(const wchar_t* moduleName);

namespace groad::cef
{
struct StartupResult
{
    bool shouldExit;
    int exitCode;
};

StartupResult initialize(HINSTANCE hInstance);
void shutdown();
void setDisableGpuVsyncSwitch(bool disableGpuVsync);
}

enum ParseResult
{
    DEFAULT,
    STOP_PROGRAM
};

constexpr const char* helpMessage = 
    PROJECT_NAME " [--help|-h] [--graphics|-g <Graphics API>] [--index|-i <Device Index>] [--vsync|-v <on|off>]: \n"
    "  --help|-h                Displays this help message.\n"
    "  --graphics|-g <API>      Sets the graphics API to be used.\n"
    "                           Supported APIs: DX11, D3D12\n"
    "  --index|-i <Index>       Selects the GPU device by index.\n"
    "                           Use a non-negative integer as <Index>.\n"
    "  --vsync|-v <on|off>      Sets whole-app VSync default.\n";

void applyEarlyVsyncOption(Options& options, int argc, const char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg = argv[i];
        if ((equalsIgnoreCase(arg, "--vsync") || equalsIgnoreCase(arg, "-v")) && ((i + 1) < argc))
        {
            const std::string_view value = argv[i + 1];
            if (equalsIgnoreCase(value, "on") || equalsIgnoreCase(value, "true") || equalsIgnoreCase(value, "1"))
            {
                options.isVsync = true;
            }
            else if (equalsIgnoreCase(value, "off") || equalsIgnoreCase(value, "false") || equalsIgnoreCase(value, "0"))
            {
                options.isVsync = false;
            }
        }
    }
}


ParseResult updateOptions(Options& options, int argc, const char** argv)
{
    int i = 1;

    auto getNextArg = [&]() -> std::string_view {
        if (i >= argc)
        {
            ERR("Argument parameter missing");
        }

        return std::string_view(argv[i++]);
    };

    while (i < argc)
    {
        const std::string_view arg = getNextArg();
        if (equalsIgnoreCase(arg, "--help") || equalsIgnoreCase(arg, "-h"))
        {
            std::cout << helpMessage << std::endl;
            return STOP_PROGRAM;
        }
        else if (equalsIgnoreCase(arg, "--graphics") || equalsIgnoreCase(arg, "-g"))
        {
            static bool isApiSet{};

            if (isApiSet)
            {
                ERR("--graphics/g can only be set once");
            }
            options.api = getNextArg();
            isApiSet = true;
        }
        else if (equalsIgnoreCase(arg, "--index") || equalsIgnoreCase(arg, "-i"))
        {
            std::string_view stringViewGpuIndex = getNextArg();
            auto [_, ec] = std::from_chars(stringViewGpuIndex.data(), stringViewGpuIndex.data() + stringViewGpuIndex.size(), options.gpuIndex);

            if (ec != std::errc())
            {
                ERR("--index/i invalid gpu index");
            }
        }
        else if (equalsIgnoreCase(arg, "--vsync") || equalsIgnoreCase(arg, "-v"))
        {
            const std::string_view value = getNextArg();
            if (equalsIgnoreCase(value, "on") || equalsIgnoreCase(value, "true") || equalsIgnoreCase(value, "1"))
            {
                options.isVsync = true;
            }
            else if (equalsIgnoreCase(value, "off") || equalsIgnoreCase(value, "false") || equalsIgnoreCase(value, "0"))
            {
                options.isVsync = false;
            }
            else
            {
                ERR("--vsync/v expects on/off, true/false or 1/0");
            }
        }
        else
        {
            ERR(std::string("Unknown argument: ") + arg.data());
        }
    }

    return DEFAULT;
}

int main(int argc, const char** argv)
{
    try
    {
        Options startupOptions;
        applyEarlyVsyncOption(startupOptions, argc, argv);
        groad::cef::setDisableGpuVsyncSwitch(!startupOptions.isVsync);

        const groad::cef::StartupResult cefStartup = groad::cef::initialize(GetModuleHandleW(nullptr));
        if (cefStartup.shouldExit)
        {
            return cefStartup.exitCode;
        }

        struct CefShutdownGuard
        {
            ~CefShutdownGuard()
            {
                groad::cef::shutdown();
            }
        } cefShutdownGuard;

        Options options;
        if (updateOptions(options, argc, argv) == STOP_PROGRAM)
        {
            return 0;
        }
        const auto parsedOptions = options.parse();

        App app = std::move(parsedOptions);

        app.init();
        app.run();
    }
    catch (const std::exception& e)
    {
        log(std::string("STL exception: ") + e.what(), LogLevel::ERROR);
        return 1;
    }
    catch (...)
    {
        log("Unknown exception", LogLevel::ERROR);
        return 1;
    }

    return 0;
}
