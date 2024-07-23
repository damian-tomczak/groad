module;

#include <wrl/client.h>
#include <d3d11.h>
#include <DirectXMath.h>

export module dxrenderer;
export import core.renderer;

import std.core;

using namespace Microsoft::WRL;
using namespace DirectX;

export
template <typename IBuffer>
struct CB : ICB
{
    ComPtr<ID3D11Buffer> buffer;
    char padding[8];
};

export class DXRenderer : public IRenderer
{
public:
    DXRenderer(std::weak_ptr<IWindow> pWindow) : IRenderer{pWindow}
    {
    }

public:
    struct GlobalCB : public CB<ID3D11Buffer>
    {
        XMMATRIX modelMtx;
        XMMATRIX view;
        XMMATRIX invView;
        XMMATRIX proj;
        XMMATRIX invProj;
        XMMATRIX texMtx;
        XMVECTOR cameraPos;
        Color color;
        int flags;
        int screenWidth;
        int screenHeight;
    } mGlobalCB;
};