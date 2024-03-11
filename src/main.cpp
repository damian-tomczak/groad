#include "utils.h"

#include <cstdlib>

import core.options;
import app;

enum ParseResult
{
    DEFAULT,
    STOP_PROGRAM
};

// clang-format off
constexpr const char* helpMessage{
    PROJECT_NAME " [--graphics|-g <Graphics API>]: \n"
    "Graphics APIs:            "
    "DX11, D3D12"
};
// clang-format on

ParseResult updateOptions(Options& options, int argc, const char** argv)
{
    int i = 1;

    const auto getNextArg = [&] {
        if (i >= argc)
        {
            ERR("Argument parameter missing");
        }

        return std::string_view(argv[i++]);
    };

    while (i < argc)
    {
        const auto arg{getNextArg()};
        if (equalsIgnoreCase(arg, "--graphics") || equalsIgnoreCase(arg, "-g"))
        {
            static bool isApiSet{};

            if (isApiSet)
            {
                ERR("--graphics/g can only be set once");
            }
            options.api = getNextArg();
            isApiSet = true;
        }
        else if (equalsIgnoreCase(arg, "--help") || equalsIgnoreCase(arg, "-h"))
        {
            std::cout << helpMessage << std::endl;
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