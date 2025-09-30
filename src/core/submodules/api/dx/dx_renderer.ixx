module;

#include "utils.h"

#include <wrl/client.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <DirectXMath.h>
#include <cstring>

export module dxrenderer;
export import core.renderer;

import std;

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
    using Device = ID3D11Device;
    using DeviceContext = ID3D11DeviceContext;
    using RenderTargetView = ID3D11RenderTargetView;
    using ShaderResourceView = ID3D11ShaderResourceView;
    using DepthStencilView = ID3D11DepthStencilView;
    using InputLayout = ID3D11InputLayout;
    using Viewport = D3D11_VIEWPORT;

    template <typename ShaderType>
    using Shader = std::pair<ComPtr<ShaderType>, ComPtr<ID3DBlob>>;

    struct Shaders
    {
        Shader<ID3D11VertexShader> defaultVS;
        Shader<ID3D11PixelShader> defaultPS;

        Shader<ID3D11VertexShader> gridVS;
        Shader<ID3D11PixelShader> gridPS;

        Shader<ID3D11VertexShader> cursorVS;
        Shader<ID3D11GeometryShader> cursorGS;
        Shader<ID3D11PixelShader> cursorPS;

        Shader<ID3D11VertexShader> bezierCurveVS;
        Shader<ID3D11HullShader> bezierCurveHS;
        Shader<ID3D11DomainShader> bezierCurveDS;
        Shader<ID3D11GeometryShader> bezierCurveBorderGS;
        Shader<ID3D11PixelShader> bezierCurvePS;

        Shader<ID3D11VertexShader> bezierPatchC0VS;
        Shader<ID3D11HullShader> bezierPatchC0HS;
        Shader<ID3D11DomainShader> bezierPatchC0DS;
        Shader<ID3D11PixelShader> bezierPatchC0PS;

        Shader<ID3D11VertexShader> bezierPatchC2VS;
        Shader<ID3D11HullShader> bezierPatchC2HS;
        Shader<ID3D11DomainShader> bezierPatchC2DS;
        Shader<ID3D11GeometryShader> bezierPatchC2GS;
        Shader<ID3D11PixelShader> bezierPatchC2PS;

        Shader<ID3D11VertexShader> waterSurfaceVS;
        Shader<ID3D11PixelShader> waterSurfacePS;

        Shader<ID3D11VertexShader> wallsVS;
        Shader<ID3D11PixelShader> wallsPS;

        Shader<ID3D11VertexShader> duckVS;
        Shader<ID3D11PixelShader> duckPS;

        Shader<ID3D11VertexShader> gableBorderVS;
        Shader<ID3D11PixelShader> gableBorderPS;

        Shader<ID3D11VertexShader> gableVS;
        Shader<ID3D11HullShader> gableHS;
        Shader<ID3D11DomainShader> gableDS;
        Shader<ID3D11PixelShader> gablePS;

        Shader<ID3D11VertexShader> billboardVS;
        Shader<ID3D11PixelShader> billboardPS;

        Shader<ID3D11VertexShader> blendVS;
        Shader<ID3D11PixelShader> blendPS;
    };

    DXRenderer(std::weak_ptr<IWindow> pWindow) : IRenderer{pWindow}
    {
    }

    virtual ~DXRenderer() = default;

    const Shaders& getShaders() const
    {
        return mShaders;
    }

    virtual Device* getDevice() const = 0;
    virtual DeviceContext* getContext() const = 0;
    virtual RenderTargetView* getRenderTargetView() const = 0;
    virtual RenderTargetView* const* getAddressOfRenderTargetView() const = 0;
    virtual IDXGISwapChain* getSwapchain() const = 0;
    virtual DepthStencilView* getDepthStencilView() const = 0;
    virtual InputLayout* getDefaultInputLayout() const = 0;
    virtual InputLayout* getBezierCurveInputLayout() const = 0;
    virtual InputLayout* getBezierPatchInputLayout() const = 0;
    virtual const Viewport* getAddressOfScreenViewport() const = 0;
    virtual ComPtr<ShaderResourceView> createShaderResourceView(const fs::path& texPath) const = 0;
    virtual void setSolidRaster(bool isCull = false) = 0;
    virtual void setWireframeRaster(bool isCull = false) = 0;
    virtual void buildGeometryBuffers() = 0;
    virtual void createShaders() = 0;
    virtual ID3D11Buffer* const* getVertexBufferAddress(size_t index) const = 0;
    virtual ID3D11Buffer* getIndexBuffer(size_t index) const = 0;

    template <typename Buffer>
    void createCB(Buffer& cb)
    {
        static_assert(std::is_base_of_v<CB<ID3D11Buffer>, Buffer>, "CB must inherit from DX constant buffer");

        auto* const pDevice = getDevice();
        ASSERT(pDevice != nullptr);

        const size_t cbSize = sizeof(Buffer);
        const size_t iSize = sizeof(CB<ID3D11Buffer>);
        const size_t byteWidth = cbSize - iSize;

        const D3D11_BUFFER_DESC desc{
            .ByteWidth = static_cast<UINT>(byteWidth),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };

        const D3D11_SUBRESOURCE_DATA initData{
            .pSysMem = reinterpret_cast<void*>(reinterpret_cast<char*>(&cb) + iSize),
        };

        HR(pDevice->CreateBuffer(&desc, &initData, cb.buffer.GetAddressOf()));
    }

    template <typename Buffer>
    void updateCB(const Buffer& cb)
    {
        static_assert(std::is_base_of_v<CB<ID3D11Buffer>, Buffer>, "Buffer must inherit from DX constant buffer");

        auto* const pContext = getContext();
        ASSERT(pContext != nullptr);

        D3D11_MAPPED_SUBRESOURCE cbData{};
        HR(pContext->Map(cb.buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData));
        const char* baseAddress = reinterpret_cast<const char*>(&cb);
        memcpy(cbData.pData, baseAddress + sizeof(CB<ID3D11Buffer>), sizeof(Buffer) - sizeof(CB<ID3D11Buffer>));
        pContext->Unmap(cb.buffer.Get(), 0);
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
        int tesFactor;
    } mGlobalCB;

protected:
    Shaders mShaders;
};
