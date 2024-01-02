module;

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <d3d11.h>

export module dxrenderer;
export import renderer;

import std.core;

using namespace DirectX;
namespace fs = std::filesystem;

// clang-format off
export namespace Colors
{
    inline constexpr XMFLOAT4 White  {1.0f, 1.0f, 1.0f, 1.0f};
    inline constexpr XMFLOAT4 Black  {0.0f, 0.0f, 0.0f, 1.0f};
    inline constexpr XMFLOAT4 Red    {1.0f, 0.0f, 0.0f, 1.0f};
    inline constexpr XMFLOAT4 Green  {0.0f, 1.0f, 0.0f, 1.0f};
    inline constexpr XMFLOAT4 Blue   {0.0f, 0.0f, 1.0f, 1.0f};
    inline constexpr XMFLOAT4 Yellow {1.0f, 1.0f, 0.0f, 1.0f};
    inline constexpr XMFLOAT4 Cyan   {0.0f, 1.0f, 1.0f, 1.0f};
    inline constexpr XMFLOAT4 Magenta{1.0f, 0.0f, 1.0f, 1.0f};
}
// clang-format on

export class DXRenderer : public IRenderer
{
public:
    DXRenderer(std::weak_ptr<IWindow> pWindow) : IRenderer{pWindow}
    {
    }
};