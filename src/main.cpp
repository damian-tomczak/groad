#include "utils.h"

#include <cstdlib>

import core.options;
import app;

enum ParseResult
{
    DEFAULT,
    STOP_PROGRAM
};

constexpr const char* helpMessage = 
    PROJECT_NAME " [--help|-h] [--graphics|-g <Graphics API>] [--index|-i <Device Index>]: \n"
    "  --help|-h                Displays this help message.\n"
    "  --graphics|-g <API>      Sets the graphics API to be used.\n"
    "                           Supported APIs: DX11, D3D12\n"
    "  --index|-i <Index>       Selects the GPU device by index.\n"
    "                           Use a non-negative integer as <Index>.\n";


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
        else
        {
            ERR("Unknown argument: "s + arg.data());
        }
    }

    return DEFAULT;
}

int main(int argc, const char** argv)
{
    try
    {
        Options options;
        if (updateOptions(options, argc, argv) == STOP_PROGRAM)
        {
            return EXIT_SUCCESS;
        }
        const auto parsedOptions = options.parse();

        App app = std::move(parsedOptions);

        app.init();
        app.run();
    }
    catch (const std::exception& e)
    {
        log("STL exception: "s + e.what(), LogLevel::ERROR);
        return EXIT_FAILURE;
    }
    catch (...)
    {
        log("Unknown exception", LogLevel::ERROR);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}