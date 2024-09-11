module;

#include "utils.h"

#include "mg.hpp"
#include "imgui_impl_dx11.h"
#include <d3dcompiler.h>
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"

#define LIGHTS_NUM 2

export module dx11renderer;
export import dxrenderer;
import std.core;
import std.filesystem;

import platform;

export struct LightsCB : public CB<ID3D11Buffer>
{
    XMVECTOR pos[LIGHTS_NUM];
};

export class DX11Renderer : public DXRenderer
{
    static constexpr bool enable4xMsaa = true;

public:
    DX11Renderer(std::weak_ptr<IWindow> pWindow) : DXRenderer{pWindow}
    {
    }

    ~DX11Renderer()
    {
        clearRenderables();

        ImGui_ImplDX11_Shutdown();
    }

    struct Shaders
    {
        template <typename ShaderType>
        using Shader = std::pair<ComPtr<ShaderType>, ComPtr<ID3D10Blob>>;

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

    void init() override;
    void onResize() override;
    void buildGeometryBuffers();

    ComPtr<ID3D11ShaderResourceView> createShaderResourceView(const fs::path& texPath) const;

    void setSolidRaster(bool isCull = false)
    {
        if (!isCull)
        {
            mpContext->RSSetState(mpFillSolidCullNoneRaster.Get());
        }
        else
        {
            ASSERT(false);
        }
    }

    void setWireframeRaster(bool isCull = false)
    {
        if (!isCull)
        {
            mpContext->RSSetState(mpFillWireframeCullNoneRaster.Get());
        }
        else
        {
            ASSERT(false);
        }
    }

    const Shaders& getShaders() const
    {
        return mShaders;
    }

    ID3D11Device* getDevice() const
    {
        return mpDevice.Get();
    }

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

    ID3D11InputLayout* getDefaultInputLayout() const
    {
        return mpDefaultInputLayout.Get();
    }

    ID3D11InputLayout* getBezierCurveInputLayout() const
    {
        return mpBezierCurveInputLayout.Get();
    }

    ID3D11InputLayout* getBezierPatchInputLayout() const
    {
        return mpBezierPatchInputLayout.Get();
    }

    const D3D11_VIEWPORT* getAddressOfScreenViewport() const
    {
        return &mScreenViewport;
    }

    template <typename Buffer>
    void createCB(Buffer& cb)
    {
        static_assert(std::is_base_of<CB<ID3D11Buffer>, Buffer>::value, "CB must inherit from ICB");

        const size_t cbSize = sizeof(Buffer);
        const size_t iSize = sizeof(CB<ID3D11Buffer>);
        const size_t byteWidth = cbSize - iSize;

        const D3D11_BUFFER_DESC desc{
            .ByteWidth = byteWidth,
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };

        const D3D11_SUBRESOURCE_DATA initData{
            .pSysMem = reinterpret_cast<void*>(reinterpret_cast<char*>(&cb) + iSize),
        };

        HR(mpDevice->CreateBuffer(&desc, &initData, cb.buffer.GetAddressOf()));
    }

    template <typename Buffer>
    void updateCB(const Buffer& cb)
    {
        static_assert(std::is_base_of<CB<ID3D11Buffer>, Buffer>::value, "Buffer must be DX11");

        D3D11_MAPPED_SUBRESOURCE cbData;
        mpContext->Map(cb.buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
        const char* baseAddress = reinterpret_cast<const char*>(&cb);
        memcpy(cbData.pData, baseAddress + sizeof(CB<ID3D11Buffer>), sizeof(Buffer) - sizeof(CB<ID3D11Buffer>));
        mpContext->Unmap(cb.buffer.Get(), 0);
    }

    void addRenderable(std::unique_ptr<IRenderable>&& renderable) override;
    void addRenderable(std::vector<std::unique_ptr<IRenderable>>&& renderables) override
    {
        // TODO: implement
        ASSERT(false);
    }
    void removeRenderable(Id id) override;
    void removeRenderable(std::vector<Id> ids) override
    {
        // TODO: implement
        ASSERT(false);
    }
    IRenderable* getRenderable(Id id) const override;

    void createShaders();

    std::vector<ComPtr<ID3D11Buffer>> mVertexBuffers;
    std::vector<ComPtr<ID3D11Buffer>> mIndexBuffers;

private:
    void initCore();
    void buildVertexLayout();
    void createRasterizers();

    Shaders mShaders;

    ComPtr<ID3D11Device> mpDevice;
    ComPtr<IDXGISwapChain> mpSwapChain;
    ComPtr<ID3D11Texture2D> mpDepthStencilBuffer;
    ComPtr<ID3D11DeviceContext> mpContext;
    ComPtr<ID3D11RenderTargetView> mpRenderTargetView;
    ComPtr<ID3D11DepthStencilView> mpDepthStencilView;

    ComPtr<ID3D11InputLayout> mpDefaultInputLayout;
    ComPtr<ID3D11InputLayout> mpBezierCurveInputLayout;
    ComPtr<ID3D11InputLayout> mpBezierPatchInputLayout;

    D3D11_VIEWPORT mScreenViewport{};
    UINT m4xMsaaQuality{};

    ComPtr<ID3D11RasterizerState> mpFillSolidCullNoneRaster;
    ComPtr<ID3D11RasterizerState> mpFillWireframeCullNoneRaster;

    // TODO: bitset
    //bool mRebuildBuffers{};
};

module :private;

void DX11Renderer::init()
{
    initCore();
    onResize();

    createShaders();
    buildVertexLayout();
    createRasterizers();

    ImGui_ImplDX11_Init(mpDevice.Get(), mpContext.Get());
}

void DX11Renderer::onResize()
{
    ASSERT(mpContext);
    ASSERT(mpDevice);
    ASSERT(mpSwapChain);

    mpRenderTargetView.Reset();
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
    HR(mpDevice->CreateBlendState(&blendDesc, &pBlendState));

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

    ComPtr<IDXGIFactory> pFactory{};
    CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);

    std::vector<ComPtr<IDXGIAdapter>> adapters;
    IDXGIAdapter* pAdapter;
    for (UINT i = 0; pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        adapters.push_back(pAdapter);
    }

    if (adapters.size() == 0)
    {
        ERR("No capable device detected to render!");
    }

    IDXGIAdapter* selectedAdapter = adapters[0].Get();

    DXGI_ADAPTER_DESC adapterDesc;
    if (SUCCEEDED(selectedAdapter->GetDesc(&adapterDesc)))
    {
        WLOG(std::format(L"Selected GPU: {}", adapterDesc.Description).data());
    }
    else
    {
        WARN("Couldn't fetch info about selected GPU");
    }

    D3D_FEATURE_LEVEL featureLevel{};
    HR(D3D11CreateDevice(selectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, createDeviceFlags, nullptr, 0,
                         D3D11_SDK_VERSION,
                         &mpDevice,
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
    for (unsigned i = 0; i < mRenderablePtrs.size(); ++i)
    {
        auto& pRenderable = mRenderablePtrs[i];

        if (pRenderable == nullptr)
        {
            continue;
        }

        if (!pRenderable->mShouldRebuildBuffers)
        {
            continue;
        }

        const std::vector<XMFLOAT3>& geometry = pRenderable->getGeometry();
        const std::vector<unsigned>& topology = pRenderable->getTopology();

        if (geometry.size() == 0)
        {
            // So far there shouldn't be such case
            ASSERT(false);
            continue;
        }

        mVertexBuffers[i].Reset();
        mIndexBuffers[i].Reset();

        D3D11_BUFFER_DESC vbd{
            .ByteWidth = static_cast<UINT>(sizeof(XMFLOAT3) * geometry.size()),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = 0,
            .StructureByteStride = 0,
        };
        D3D11_SUBRESOURCE_DATA vinitData{
            .pSysMem = geometry.data(),
        };
        HR(mpDevice->CreateBuffer(&vbd, &vinitData, &mVertexBuffers[i]));

        if (topology.size() == 0)
        {
            continue;
        }

        D3D11_BUFFER_DESC ibd{
            .ByteWidth = static_cast<UINT>(sizeof(unsigned) * topology.size()),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_INDEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = 0,
            .StructureByteStride = 0,
        };
        D3D11_SUBRESOURCE_DATA initData{
            .pSysMem = topology.data(),
        };
        HR(mpDevice->CreateBuffer(&ibd, &initData, &mIndexBuffers[i]));

        pRenderable->mShouldRebuildBuffers = false;
    }
}

void DX11Renderer::createShaders()
{
    bool isSuccess = true;

    static bool firstCall = true;

    auto checkShader = [](const HRESULT hr) {
        if (FAILED(hr))
        {
            const std::string msg =
                std::format("Shader creation error: {}({})", std::system_category().message(hr), hr);

            if (firstCall)
            {
                ERR(msg);
            }
            else
            {
                WARN(msg);
            }
        }
    };

    auto compileShader = [&isSuccess, checkShader](const fs::path shaderPath, const std::string_view shaderModel,
                             ComPtr<ID3D10Blob>& shaderBlob, std::function<HRESULT()> creationShaderFunction) {
        shaderBlob.Reset();

        static constexpr std::string_view shaderEntryPoint{"main"};

        // TODO: fix it
        const fs::path fullShaderPath = SHADERS_PATH / shaderPath;

        if (!fs::exists(fullShaderPath))
        {
            ERR(("Shaders compiler error: following path doesn't exist " + fullShaderPath.string()).c_str());
        }

        ComPtr<ID3D10Blob> compilationMsgs;
        DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifndef NDEBUG
        shaderFlags |= D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

        const HRESULT hr =
            D3DCompileFromFile(fullShaderPath.c_str(), nullptr, nullptr, shaderEntryPoint.data(), shaderModel.data(),
                               shaderFlags, 0, shaderBlob.GetAddressOf(), compilationMsgs.GetAddressOf());

        const bool isContainingMessage = compilationMsgs != nullptr;

        if (FAILED(hr))
        {
            bool isWarning = false;
            std::string message;

            if (isContainingMessage)
            {
                message = reinterpret_cast<char*>(compilationMsgs->GetBufferPointer());
                if (message.contains(": warning"))
                {
                    isWarning = true;
                }
            }

            std::ostringstream errorMessage;
            errorMessage << "Shader compilation " << (isWarning ? "warning" : "error") << ": "
                         << std::system_category().message(hr) << ")[" << hr << "] "
                         << (isContainingMessage ? message : "ERROR DIDN'T PROVIDE EXPLANATION");

            if (firstCall && !isWarning)
            {
                ERR(errorMessage.str());
            }
            else if (isWarning)
            {
                WARN(errorMessage.str());
            }
            else
            {
                ERR_NOTERMINATE(errorMessage.str());
            }
        }

        compilationMsgs.Reset();

        if (shaderBlob != nullptr)
        {
            checkShader(creationShaderFunction());
        }
        else
        {
            isSuccess = false;
        }
    };

    #include "shaders_loader.inl"

    firstCall = false;

    if (isSuccess)
    {
        LOG("Shader compilaton SUCCESS");
    }
    else
    {
        ERR_NOTERMINATE("Shader compilation ERROR");
    }

}

void DX11Renderer::buildVertexLayout()
{
    {
        std::array desc
        {
            D3D11_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        HR(mpDevice->CreateInputLayout(desc.data(), static_cast<UINT>(desc.size()), mShaders.defaultVS.second->GetBufferPointer(),
            mShaders.defaultVS.second->GetBufferSize(), &mpDefaultInputLayout));
    }

    {
        std::array desc
        {
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                  D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 3 * sizeof(float),  D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 2, DXGI_FORMAT_R32G32B32_FLOAT, 0, 6 * sizeof(float),  D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 3, DXGI_FORMAT_R32G32B32_FLOAT, 0, 9 * sizeof(float),  D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        HR(mpDevice->CreateInputLayout(desc.data(), static_cast<UINT>(desc.size()),
            mShaders.bezierCurveVS.second->GetBufferPointer(), mShaders.bezierCurveVS.second->GetBufferSize(), &mpBezierCurveInputLayout));
    }

    {
        static constexpr size_t sizeOfR32G32B32Format = 3 * sizeof(float);

        std::array desc
        {
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 0,  DXGI_FORMAT_R32G32B32_FLOAT, 0, 0  * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 1,  DXGI_FORMAT_R32G32B32_FLOAT, 0, 1  * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 2,  DXGI_FORMAT_R32G32B32_FLOAT, 0, 2  * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 3,  DXGI_FORMAT_R32G32B32_FLOAT, 0, 3  * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 4,  DXGI_FORMAT_R32G32B32_FLOAT, 0, 4  * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 5,  DXGI_FORMAT_R32G32B32_FLOAT, 0, 5  * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 6,  DXGI_FORMAT_R32G32B32_FLOAT, 0, 6  * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 7,  DXGI_FORMAT_R32G32B32_FLOAT, 0, 7  * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 8,  DXGI_FORMAT_R32G32B32_FLOAT, 0, 8  * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 9,  DXGI_FORMAT_R32G32B32_FLOAT, 0, 9  * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 10, DXGI_FORMAT_R32G32B32_FLOAT, 0, 10 * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 11, DXGI_FORMAT_R32G32B32_FLOAT, 0, 11 * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 12, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12 * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 13, DXGI_FORMAT_R32G32B32_FLOAT, 0, 13 * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 14, DXGI_FORMAT_R32G32B32_FLOAT, 0, 14 * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
            D3D11_INPUT_ELEMENT_DESC{"CONTROL_POINT", 15, DXGI_FORMAT_R32G32B32_FLOAT, 0, 15 * sizeOfR32G32B32Format, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        HR(mpDevice->CreateInputLayout(desc.data(), static_cast<UINT>(desc.size()),
            mShaders.bezierPatchC0VS.second->GetBufferPointer(), mShaders.bezierPatchC0VS.second->GetBufferSize(), &mpBezierPatchInputLayout));
    }
}

void DX11Renderer::createRasterizers()
{
    D3D11_RASTERIZER_DESC rasterDesc
    {
        .FillMode = D3D11_FILL_SOLID,
        .CullMode = D3D11_CULL_NONE,
        .FrontCounterClockwise = false,
        .DepthBias = 0,
        .DepthBiasClamp = 0.0f,
        .SlopeScaledDepthBias = 0.0f,
        .DepthClipEnable = true,
        .ScissorEnable = false,
        .MultisampleEnable = false,
        .AntialiasedLineEnable = false,
    };

    HR(mpDevice->CreateRasterizerState(&rasterDesc, mpFillSolidCullNoneRaster.GetAddressOf()));

    rasterDesc.FillMode = D3D11_FILL_WIREFRAME;

    HR(mpDevice->CreateRasterizerState(&rasterDesc, mpFillWireframeCullNoneRaster.GetAddressOf()));
}

ComPtr<ID3D11ShaderResourceView> DX11Renderer::createShaderResourceView(const fs::path& texPath) const
{
    ID3D11ShaderResourceView* pRv;

    if (texPath.extension() == ".dds")
    {
        HR(CreateDDSTextureFromFile(mpDevice.Get(), mpContext.Get(), texPath.c_str(), nullptr, &pRv));
    }
    else
    {
        HR(CreateWICTextureFromFile(mpDevice.Get(), mpContext.Get(), texPath.c_str(), nullptr, &pRv));
    }

    ComPtr<ID3D11ShaderResourceView> resourceView(pRv);

    return resourceView;
}

void DX11Renderer::addRenderable(std::unique_ptr<IRenderable>&& pRenderable)
{
    const Id renderableId = pRenderable->getId();
    ASSERT((pRenderable != nullptr) && (renderableId != invalidId));

    if (static_cast<size_t>(renderableId) <= mRenderablePtrs.size())
    {
        mRenderablePtrs.emplace_back(std::move(pRenderable));
        mVertexBuffers.emplace_back();
        mIndexBuffers.emplace_back();
    }
    else
    {
        // Expected only while loading scene from serializer.
        static constexpr size_t reasonablyEmptyAmountPredecessors = 500;
        ASSERT(!((renderableId - mRenderablePtrs.size()) > reasonablyEmptyAmountPredecessors));

        mRenderablePtrs.resize(renderableId + 1);
        mRenderablePtrs[renderableId] = std::move(pRenderable);
        mVertexBuffers.resize(renderableId + 1, nullptr);
        mIndexBuffers.resize(renderableId + 1, nullptr);
    }

    mRenderablePtrs[renderableId]->regenerateData();
}

void DX11Renderer::removeRenderable(Id id)
{
    if (id == invalidId)
    {
        ASSERT(false);
        return;
    }

    ASSERT(mRenderablePtrs.size() > static_cast<size_t>(id));

    (mRenderablePtrs.begin() + id)->reset();
    (mVertexBuffers.begin() + id)->Reset();
    (mIndexBuffers.begin() + id)->Reset();

    IIdentifiable::mFreeIds.push(id);
}

IRenderable* DX11Renderer::getRenderable(Id id) const
{
    if ((id == invalidId) || (static_cast<size_t>(id) > mRenderablePtrs.size()))
    {
        ASSERT(false);
        return nullptr;
    }

    return mRenderablePtrs[id].get();
}