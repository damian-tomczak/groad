module;

#include "mg.hpp"
#include <DirectXPackedVector.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include "imgui_impl_dx11.h"

#include "utils.h"

export module dx11renderer;
export import dxrenderer;
import std.core;
import std.filesystem;

import platform;

using namespace Microsoft::WRL;
using namespace DirectX;

export struct ConstantBufferData
{
    XMMATRIX model;
    XMMATRIX view;
    XMMATRIX invView;
    XMMATRIX proj;
    XMMATRIX invProj;
    bool isSelected;
    char padding[15]; // TODO: alignas doesn't work why?
};
static_assert(sizeof(ConstantBufferData) % 16 == 0);

export class DX11Renderer : public DXRenderer
{
    static constexpr bool enable4xMsaa = true;
    static constexpr D3D_DRIVER_TYPE d3dDriverType{D3D_DRIVER_TYPE_HARDWARE};

public:
    DX11Renderer(std::weak_ptr<IWindow> pWindow) : DXRenderer{pWindow}
    {
    }

    ~DX11Renderer()
    {
        ImGui_ImplDX11_Shutdown();
    }

    void init() override;
    void onResize() override;
    void buildGeometryBuffers();

    std::vector<std::shared_ptr<Renderable>> mRenderables;

    ComPtr<ID3D11VertexShader> mpVS;
    ComPtr<ID3D11PixelShader> mpPS;
    ComPtr<ID3D11VertexShader> mpGridVS;
    ComPtr<ID3D11PixelShader> mpGridPS;
    ComPtr<ID3D11VertexShader> mpCursorVS;
    ComPtr<ID3D11PixelShader> mpCursorPS;
    std::vector<ComPtr<ID3D11Buffer>> mVertexBuffers;
    std::vector<ComPtr<ID3D11Buffer>> mIndexBuffers;

    ID3D11DeviceContext* getContext() const
    {
        return mpContext.Get();
    }

    ID3D11RenderTargetView* getRenderTargetView() const
    {
        return mpRenderTargetView.Get();
    }

    ID3D11RenderTargetView* const* getAddressOfRenderTargetView() const
    {
        return mpRenderTargetView.GetAddressOf();
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

    void addRenderable(std::unique_ptr<Renderable>&& renderable)
    {
        mRenderables.emplace_back(std::move(renderable));
        mVertexBuffers.emplace_back();
        mIndexBuffers.emplace_back();

        //mRebuildBuffers = true;
    }

    void removeRenderable(int idx)
    {
        if (idx != -1)
        {
            mRenderables.erase(mRenderables.begin() + idx);
            mVertexBuffers.erase(mVertexBuffers.begin() + idx);
            mIndexBuffers.erase(mIndexBuffers.begin() + idx);
        }
    }

    std::shared_ptr<Renderable> getRenderable(int idx) const
    {
        if (idx != -1)
        {
            return mRenderables.at(idx);
        }

        return nullptr;
    }

private:
    void initCore();
    void createShaders();
    void buildVertexLayout();

    ComPtr<ID3D11Device> mpDevice;
    ComPtr<IDXGISwapChain> mpSwapChain;
    ComPtr<ID3D11Texture2D> mpDepthStencilBuffer;
    ComPtr<ID3D11DeviceContext> mpContext;
    ComPtr<ID3D11RenderTargetView> mpRenderTargetView;
    ComPtr<ID3D11DepthStencilView> mpDepthStencilView;

    ComPtr<ID3D11RasterizerState> mpWireframeRS;
    ComPtr<ID3D10Blob> mpVSBlob;
    ComPtr<ID3D11Buffer> mpConstantBuffer;
    ComPtr<ID3D11InputLayout> mpInputLayout;

    D3D11_VIEWPORT mScreenViewport{};
    UINT m4xMsaaQuality{};

    //bool mRebuildBuffers{};
};

module :private;

void DX11Renderer::init()
{
    initCore();
    onResize();

    createShaders();
    buildVertexLayout();

    D3D11_RASTERIZER_DESC wireframeDesc{
        .FillMode = D3D11_FILL_SOLID,
        .CullMode = D3D11_CULL_NONE,
        .FrontCounterClockwise = false,
        .DepthClipEnable = true,
    };

    HR(mpDevice->CreateRasterizerState(&wireframeDesc, &mpWireframeRS));

    ImGui_ImplDX11_Init(mpDevice.Get(), mpContext.Get());
}

void DX11Renderer::onResize()
{
    ASSERT(mpContext);
    ASSERT(mpDevice);
    ASSERT(mpSwapChain);

    mpRenderTargetView.Reset();
    mpDepthStencilView.Reset();
    mpDepthStencilBuffer.Reset();

    const auto pWindow = mpWindow.lock().get();
    ASSERT(pWindow);

    ComPtr<ID3D11Texture2D> pBackBuffer;
    HR(mpSwapChain->ResizeBuffers(1, pWindow->getWidth(), pWindow->getHeight(), DXGI_FORMAT_R8G8B8A8_UNORM, 0));
    HR(mpSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(pBackBuffer.GetAddressOf())));
    HR(mpDevice->CreateRenderTargetView(pBackBuffer.Get(), 0, mpRenderTargetView.GetAddressOf()));

    D3D11_TEXTURE2D_DESC depthStencilDesc{
        .Width = static_cast<UINT>(pWindow->getWidth()),
        .Height = static_cast<UINT>(pWindow->getHeight()),
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
        .SampleDesc{.Count = enable4xMsaa ? 4 : 1, .Quality = enable4xMsaa ? m4xMsaaQuality - 1 : 0},
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_DEPTH_STENCIL,
    };

    HR(mpDevice->CreateTexture2D(&depthStencilDesc, 0, mpDepthStencilBuffer.GetAddressOf()));
    HR(mpDevice->CreateDepthStencilView(mpDepthStencilBuffer.Get(), 0, mpDepthStencilView.GetAddressOf()));

    mpContext->OMSetRenderTargets(1, mpRenderTargetView.GetAddressOf(), mpDepthStencilView.Get());

    mScreenViewport.TopLeftX = 0;
    mScreenViewport.TopLeftY = 0;
    mScreenViewport.Width = static_cast<float>(pWindow->getWidth());
    mScreenViewport.Height = static_cast<float>(pWindow->getHeight());
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;

    mpContext->RSSetViewports(1, &mScreenViewport);

    const D3D11_BLEND_DESC blendDesc
    {
        .RenderTarget =
        {{
            .BlendEnable = true,
            .SrcBlend = D3D11_BLEND_SRC_ALPHA,
            .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
            .BlendOp = D3D11_BLEND_OP_ADD,
            .SrcBlendAlpha = D3D11_BLEND_ONE,
            .DestBlendAlpha = D3D11_BLEND_ZERO,
            .BlendOpAlpha = D3D11_BLEND_OP_ADD,
            .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
        }},
    };

    ComPtr<ID3D11BlendState> pBlendState;
    HR(mpDevice->CreateBlendState(&blendDesc, pBlendState.GetAddressOf()));

    float blendFactor[]{0.0f, 0.0f, 0.0f, 0.0f};
    UINT sampleMask = 0xffffffff;
    mpContext->OMSetBlendState(pBlendState.Get(), blendFactor, sampleMask);
}

void DX11Renderer::initCore()
{
    UINT createDeviceFlags = 0;
#ifndef NDEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel{};
    HR(D3D11CreateDevice(0, d3dDriverType, 0, createDeviceFlags, 0, 0, D3D11_SDK_VERSION, mpDevice.GetAddressOf(),
                         &featureLevel, mpContext.GetAddressOf()));

    if (featureLevel != D3D_FEATURE_LEVEL_11_0)
    {
        ERR("Direct11 unsupported");
    }

    mpDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 8, &m4xMsaaQuality); // 4x Msaa is always supported
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
    HR(mpDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(dxgiDevice.GetAddressOf())));

    ComPtr<IDXGIAdapter> dxgiAdapter;
    HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(dxgiAdapter.GetAddressOf())));

    ComPtr<IDXGIFactory> dxgiFactory;
    HR(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(dxgiFactory.GetAddressOf())));

    HR(dxgiFactory->CreateSwapChain(mpDevice.Get(), &sd, mpSwapChain.GetAddressOf()));

    HR(dxgiFactory->MakeWindowAssociation(sd.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES));
}

void DX11Renderer::buildGeometryBuffers()
{
    for (unsigned int i{}; i < mRenderables.size(); ++i)
    {
        //if (mRenderables[i] == nullptr)
        //{
        //    continue;
        //}

        mVertexBuffers[i].Reset();
        mIndexBuffers[i].Reset();

        D3D11_BUFFER_DESC vbd{
            .ByteWidth = static_cast<UINT>(sizeof(float) * mRenderables[i]->getGeometry().size()),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = 0,
            .StructureByteStride = 0,
        };
        D3D11_SUBRESOURCE_DATA vinitData{
            .pSysMem = mRenderables[i]->getGeometry().data(),
        };
        HR(mpDevice->CreateBuffer(&vbd, &vinitData, mVertexBuffers[i].GetAddressOf()));

        D3D11_BUFFER_DESC ibd{
            .ByteWidth = static_cast<UINT>(sizeof(unsigned) * mRenderables[i]->getTopology().size()),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_INDEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = 0,
            .StructureByteStride = 0,
        };
        D3D11_SUBRESOURCE_DATA initData{
            .pSysMem = mRenderables[i]->getTopology().data(),
        };
        HR(mpDevice->CreateBuffer(&ibd, &initData, mIndexBuffers[i].GetAddressOf()));
    }

    //mRebuildBuffers = false;
}


void DX11Renderer::createShaders()
{
    namespace fs = std::filesystem;

    auto compileShader = [](const fs::path shaderPath, const std::string_view shaderModel, ComPtr<ID3D10Blob>& shaderBlob) {
        static constexpr std::string_view shaderEntryPoint{"main"};
        static const fs::path shaderPaths{ASSETS_PATH "shaders/"};

        const fs::path fullShaderPath = shaderPaths / shaderPath;

        ComPtr<ID3D10Blob> compilationMsgs;
        DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifndef NDEBUG
        shaderFlags |= D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

        HRESULT hr = D3DCompileFromFile(
            fullShaderPath.c_str(),
            nullptr,
            nullptr,
            shaderEntryPoint.data(),
            shaderModel.data(),
            shaderFlags,
            0,
            shaderBlob.GetAddressOf(),
            compilationMsgs.GetAddressOf());

        const bool containMessage = compilationMsgs != nullptr;

        if (FAILED(hr) || containMessage)
        {
            std::ostringstream errorMessage;
            errorMessage << "Shader compilation error (" << std::system_category().message(hr) << ")[" << hr << "]"
                         << (containMessage ? ":" : "")
                         << (containMessage ? reinterpret_cast<char*>(compilationMsgs->GetBufferPointer()) : "");

            ERR(errorMessage.str());
        }

        compilationMsgs.Reset();
    };

    ComPtr<ID3D10Blob> compiledShader;

    compileShader("torus_vs.hlsl", "vs_5_0", mpVSBlob);
    HR(mpDevice->CreateVertexShader(mpVSBlob->GetBufferPointer(), mpVSBlob->GetBufferSize(), nullptr, mpVS.GetAddressOf()));

    compileShader("torus_ps.hlsl", "ps_5_0", compiledShader);
    HR(mpDevice->CreatePixelShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), nullptr, &mpPS));

    compileShader("infinite_grid_vs.hlsl", "vs_5_0", compiledShader);
    HR(mpDevice->CreateVertexShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), nullptr,
                                    mpGridVS.GetAddressOf()));

    compileShader("infinite_grid_ps.hlsl", "ps_5_0", compiledShader);
    HR(mpDevice->CreatePixelShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), nullptr,
                                   &mpGridPS));

    compileShader("cursor_vs.hlsl", "vs_5_0", compiledShader);
    HR(mpDevice->CreateVertexShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), nullptr,
                                    mpCursorVS.GetAddressOf()));

    compileShader("cursor_ps.hlsl", "ps_5_0", compiledShader);
    HR(mpDevice->CreatePixelShader(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), nullptr,
                                   &mpCursorPS));

    D3D11_BUFFER_DESC constantBufferDesc
    {
        .ByteWidth = sizeof(ConstantBufferData),
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    };
    HR(mpDevice->CreateBuffer(&constantBufferDesc, nullptr, &mpConstantBuffer));
}


void DX11Renderer::buildVertexLayout()
{
    std::array vertexDesc
    {
        D3D11_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    HR(mpDevice->CreateInputLayout(vertexDesc.data(), static_cast<UINT>(vertexDesc.size()), mpVSBlob->GetBufferPointer(),
        mpVSBlob->GetBufferSize(), &mpInputLayout));
}