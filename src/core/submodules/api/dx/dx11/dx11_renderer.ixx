module;

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include "utils.hpp"

export module dx11renderer;
import std.core;
import dxrenderer;
import platform;

using namespace Microsoft::WRL;
using namespace DirectX;

export class DX11Renderer : public DXRenderer
{
    static constexpr bool enable4xMsaa = true;
    static constexpr D3D_DRIVER_TYPE d3dDriverType{D3D_DRIVER_TYPE_HARDWARE};

public:
    DX11Renderer(std::weak_ptr<IWindow> pWindow) : DXRenderer{pWindow}
    {
    }

    void init() override;
    void onResize() override;

    ID3D11DeviceContext* getImmediateContext() const
    {
        return mpD3DImmediateContext.Get();
    }

    ID3D11RenderTargetView* getRenderTargetView() const
    {
        return mpRenderTargetView.Get();
    }

    IDXGISwapChain* getSwapchain() const
    {
        return mpSwapChain.Get();
    }

    ID3D11DepthStencilView* getDepthStencilView() const
    {
        return mpDepthStencilView.Get();
    }

    ID3D11InputLayout* getInputLayout() const
    {
        return mpInputLayout.Get();
    }

    ID3D11Buffer* const* getVertexBuffer() const
    {
        return mpBoxVB.GetAddressOf();
    }

    ID3D11Buffer* const* getColorBuffer() const
    {
        return mpBoxCB.GetAddressOf();
    }

    ID3D11Buffer* getIndexBuffer() const
    {
        return mpBoxIB.Get();
    }

    ID3D11RasterizerState* getWireframeRS() const
    {
        return mpWireframeRS.Get();
    }

    ID3D11Buffer* getConstantBuffer() const
    {
        return mpConstantBuffer.Get();
    }

    ID3D11Buffer* const* getAddressOfConstantBuffer() const
    {
        return mpConstantBuffer.GetAddressOf();
    }

    ID3D11VertexShader* getVertexShader() const
    {
        return mpVS.Get();
    }

    ID3D11PixelShader* getFragmentShader() const
    {
        return mpPS.Get();
    }

private:
    void initCore();
    void buildGeometryBuffers();
    void buildFX();
    void buildVertexLayout();

    ComPtr<ID3D11Device> mpD3DDevice;
    ComPtr<IDXGISwapChain> mpSwapChain;
    ComPtr<ID3D11Texture2D> mpDepthStencilBuffer;
    ComPtr<ID3D11DeviceContext> mpD3DImmediateContext;
    ComPtr<ID3D11RenderTargetView> mpRenderTargetView;
    ComPtr<ID3D11DepthStencilView> mpDepthStencilView;

    ComPtr<ID3D11Buffer> mpBoxVB;
    ComPtr<ID3D11Buffer> mpBoxCB;
    ComPtr<ID3D11Buffer> mpBoxIB;
    ComPtr<ID3D11RasterizerState> mpWireframeRS;
    ComPtr<ID3D10Blob> mpVSBlob;
    ComPtr<ID3D11VertexShader> mpVS;
    ComPtr<ID3D11PixelShader> mpPS;
    ComPtr<ID3D11Buffer> mpConstantBuffer;
    ComPtr<ID3D11InputLayout> mpInputLayout;

    D3D11_VIEWPORT mScreenViewport{};
    UINT m4xMsaaQuality{};
};

module :private;

namespace
{
constexpr UINT argbToAbgr(XMFLOAT4 argb)
{
    int A = static_cast<int>(argb.w * 255);
    int R = static_cast<int>(argb.x * 255);
    int G = static_cast<int>(argb.y * 255);
    int B = static_cast<int>(argb.z * 255);
    return (A << 24) | (B << 16) | (G << 8) | (R << 0);
}
} // namespace

void DX11Renderer::init()
{
    initCore();

    buildGeometryBuffers();
    buildFX();
    buildVertexLayout();

    D3D11_RASTERIZER_DESC wireframeDesc{
        .FillMode = D3D11_FILL_WIREFRAME,
        .CullMode = D3D11_CULL_NONE,
        .FrontCounterClockwise = false,
        .DepthClipEnable = true,
    };

    HR(mpD3DDevice->CreateRasterizerState(&wireframeDesc, &mpWireframeRS));
}

void DX11Renderer::onResize()
{
    ASSERT(mpD3DImmediateContext);
    ASSERT(mpD3DDevice);
    ASSERT(mpSwapChain);

    mpRenderTargetView.Reset();
    mpDepthStencilView.Reset();
    mpDepthStencilBuffer.Reset();

    const auto pWindow = mpWindow.lock().get();
    if (pWindow == nullptr)
    {
        ERR("Window is invalid");
    }

    HR(mpSwapChain->ResizeBuffers(1, pWindow->getWidth(), pWindow->getHeight(), DXGI_FORMAT_R8G8B8A8_UNORM, 0));
    ComPtr<ID3D11Texture2D> pBackBuffer;
    HR(mpSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(pBackBuffer.GetAddressOf())));
    HR(mpD3DDevice->CreateRenderTargetView(pBackBuffer.Get(), 0, mpRenderTargetView.GetAddressOf()));
    pBackBuffer.Reset();

    D3D11_TEXTURE2D_DESC depthStencilDesc{
        .Width = static_cast<UINT>(pWindow->getWidth()),
        .Height = static_cast<UINT>(pWindow->getHeight()),
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
        .SampleDesc{.Count = enable4xMsaa ? 4 : 1, .Quality = enable4xMsaa ? m4xMsaaQuality - 1 : 0},
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_DEPTH_STENCIL,
        .CPUAccessFlags = 0,
        .MiscFlags = 0,
    };

    HR(mpD3DDevice->CreateTexture2D(&depthStencilDesc, 0, mpDepthStencilBuffer.GetAddressOf()));
    HR(mpD3DDevice->CreateDepthStencilView(mpDepthStencilBuffer.Get(), 0, mpDepthStencilView.GetAddressOf()));

    mpD3DImmediateContext->OMSetRenderTargets(1, mpRenderTargetView.GetAddressOf(), mpDepthStencilView.Get());

    mScreenViewport.TopLeftX = 0;
    mScreenViewport.TopLeftY = 0;
    mScreenViewport.Width = static_cast<float>(pWindow->getWidth());
    mScreenViewport.Height = static_cast<float>(pWindow->getHeight());
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;

    mpD3DImmediateContext->RSSetViewports(1, &mScreenViewport);
}

void DX11Renderer::initCore()
{
    UINT createDeviceFlags = 0;
#ifndef NDEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel{};
    HR(D3D11CreateDevice(0, d3dDriverType, 0, createDeviceFlags, 0, 0, D3D11_SDK_VERSION, mpD3DDevice.GetAddressOf(),
                         &featureLevel, mpD3DImmediateContext.GetAddressOf()));

    if (featureLevel != D3D_FEATURE_LEVEL_11_0)
    {
        ERR("Direct11 unsupported");
    }

    mpD3DDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4,
                                               &m4xMsaaQuality); // 4x Msaa is always supported
    ASSERT(m4xMsaaQuality > 0);

    const auto pWindow = dynamic_cast<Win32Window*>(mpWindow.lock().get());
    ASSERT(pWindow != nullptr);

    DXGI_SWAP_CHAIN_DESC sd{
        .BufferDesc{
            .Width = static_cast<UINT>(pWindow->getWidth()),
            .Height = static_cast<UINT>(pWindow->getHeight()),
            .RefreshRate{.Numerator = 60, .Denominator = 1},
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            .Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
        },
        .SampleDesc{.Count = enable4xMsaa ? 4u : 1u, .Quality = enable4xMsaa ? m4xMsaaQuality - 1 : 0},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = 1,
        .OutputWindow = pWindow->getHwnd(),
        .Windowed = true,
        .SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
        .Flags = 0,
    };

    ComPtr<IDXGIDevice> dxgiDevice;
    HR(mpD3DDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgiDevice.GetAddressOf())));

    ComPtr<IDXGIAdapter> dxgiAdapter;
    HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(dxgiAdapter.GetAddressOf())));

    ComPtr<IDXGIFactory> dxgiFactory;
    HR(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(dxgiFactory.GetAddressOf())));

    HR(dxgiFactory->CreateSwapChain(mpD3DDevice.Get(), &sd, mpSwapChain.GetAddressOf()));

    HR(dxgiFactory->MakeWindowAssociation(sd.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES));

    onResize();
}

void DX11Renderer::buildGeometryBuffers()
{
    // clang-format off
    XMFLOAT3 vertices[] =
    {
        XMFLOAT3(-1.0f, -1.0f, -1.0f),
        XMFLOAT3(-1.0f, +1.0f, -1.0f),
        XMFLOAT3(+1.0f, +1.0f, -1.0f),
        XMFLOAT3(+1.0f, -1.0f, -1.0f),
        XMFLOAT3(-1.0f, -1.0f, +1.0f),
        XMFLOAT3(-1.0f, +1.0f, +1.0f),
        XMFLOAT3(+1.0f, +1.0f, +1.0f),
        XMFLOAT3(+1.0f, -1.0f, +1.0f),
    };
    // clang-format on

    D3D11_BUFFER_DESC vbd{
        .ByteWidth = sizeof(XMFLOAT3) * 8,
        .Usage = D3D11_USAGE_IMMUTABLE,
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags = 0,
        .MiscFlags = 0,
        .StructureByteStride = 0,
    };
    D3D11_SUBRESOURCE_DATA vinitData{
        .pSysMem = vertices,
    };
    HR(mpD3DDevice->CreateBuffer(&vbd, &vinitData, mpBoxVB.GetAddressOf()));

    // clang-format off
    PackedVector::XMCOLOR colors[]
    {
        argbToAbgr(Colors::White  ),
        argbToAbgr(Colors::Black  ),
        argbToAbgr(Colors::Red    ),
        argbToAbgr(Colors::Green  ),
        argbToAbgr(Colors::Blue   ),
        argbToAbgr(Colors::Yellow ),
        argbToAbgr(Colors::Cyan   ),
        argbToAbgr(Colors::Magenta)
    };
    // clang-format on

    D3D11_BUFFER_DESC cbd{
        .ByteWidth = sizeof(PackedVector::XMCOLOR) * 8,
        .Usage = D3D11_USAGE_IMMUTABLE,
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags = 0,
        .MiscFlags = 0,
        .StructureByteStride = 0,
    };

    D3D11_SUBRESOURCE_DATA cinitData{
        .pSysMem = colors,
    };
    HR(mpD3DDevice->CreateBuffer(&cbd, &cinitData, mpBoxCB.GetAddressOf()));

    // clang-format off
	UINT indices[] = {
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};
    // clang-format off

    D3D11_BUFFER_DESC ibd
    {
        .ByteWidth = sizeof(UINT) * 36,
        .Usage = D3D11_USAGE_IMMUTABLE,
        .BindFlags = D3D11_BIND_INDEX_BUFFER,
        .CPUAccessFlags = 0,
        .MiscFlags = 0,
	    .StructureByteStride = 0,
    };
    D3D11_SUBRESOURCE_DATA initData
    {
        .pSysMem = indices,
    };
    HR(mpD3DDevice->CreateBuffer(&ibd, &initData, &mpBoxIB));
}

void DX11Renderer::buildFX()
{
    const auto checkCompilationErrors = [](ComPtr<ID3D10Blob> compilationMsgs) {
        if (compilationMsgs != 0)
        {
            ERR("Shader compilation error: "s + reinterpret_cast<char*>(compilationMsgs->GetBufferPointer()));
            compilationMsgs.Reset();
        }
    };

    DWORD shaderFlags{};
#ifndef NDEBUG
    shaderFlags |= D3D10_SHADER_DEBUG;
    shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3D10Blob> compiledShader;
    ComPtr<ID3D10Blob> compilationMsgs;

    HR(D3DCompileFromFile(ASSETS_PATH L"shaders/color.fx", 0, 0, "VS", "vs_5_0", shaderFlags,
        0, mpVSBlob.GetAddressOf(), compilationMsgs.GetAddressOf()));

    checkCompilationErrors(compilationMsgs);

    HR(mpD3DDevice->CreateVertexShader(mpVSBlob->GetBufferPointer(), mpVSBlob->GetBufferSize(), nullptr, mpVS.GetAddressOf()));

    HR(D3DCompileFromFile(ASSETS_PATH L"shaders/color.fx", 0, 0, "PS", "ps_5_0", shaderFlags,
        0, compiledShader.GetAddressOf(), compilationMsgs.GetAddressOf()));

    checkCompilationErrors(compilationMsgs);

    HR(mpD3DDevice->CreatePixelShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), nullptr, &mpPS));
    compiledShader.Reset();

    D3D11_BUFFER_DESC constantBufferDesc
    {
        .ByteWidth = sizeof(XMMATRIX),
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
    };
    HR(mpD3DDevice->CreateBuffer(&constantBufferDesc, nullptr, &mpConstantBuffer));
}

void DX11Renderer::buildVertexLayout()
{
    D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    HR(mpD3DDevice->CreateInputLayout(vertexDesc, 2, mpVSBlob->GetBufferPointer(),
        mpVSBlob->GetBufferSize(), &mpInputLayout));
}