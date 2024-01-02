module;

#include "utils.hpp"

export module core.options;
export import std.core;

import renderer;

API strToApi(const std::string_view& str);

export
{
    struct ParsedOptions
    {
        API api;
    };

    struct Options final
    {
        std::string_view api{"DX11"};

        [[nodiscard]] ParsedOptions parse()
        {
            return {.api{strToApi(api)}};
        }
    };

    __forceinline bool equalsIgnoreCase(const std::string_view& s1, const std::string_view& s2,
                                        const std::locale& loc = std::locale())
    {
        const std::ctype<char>& ctype = std::use_facet<std::ctype<char>>(loc);
        const auto compareCharLower = [&](char c1, char c2) { return ctype.tolower(c1) == ctype.tolower(c2); };
        return (s1.size() == s2.size()) && std::equal(s1.begin(), s1.end(), s2.begin(), compareCharLower);
    }
}

module :private;

API strToApi(const std::string_view& str)
{
    if (equalsIgnoreCase(str, "DX11"))
    {
        return API::DX11;
    }
    else if (equalsIgnoreCase(str, "DX12"))
    {
        return API::DX12;
    }
    else if (equalsIgnoreCase(str, "Vulkan"))
    {
        return API::VULKAN;
    }

    ERR("Unknown graphics api"s + str.data());

    return {};
}