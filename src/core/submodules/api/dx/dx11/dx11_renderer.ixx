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

export struct ICB
{
    ComPtr<ID3D11Buffer> buffer;
    char padding[8];
};

export struct GlobalCB : public ICB
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
};

export struct LightsCB : public ICB
{
    XMVECTOR pos[LIGHTS_NUM];
};

static_assert(sizeof(GlobalCB) % 16 == 0);

export class DX11Renderer : public DXRenderer
{
    static constexpr bool enable4xMsaa = true;

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

    // TODO: remove p prefix
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

        Shader<ID3D11VertexShader> bezierVS;
        Shader<ID3D11HullShader> bezierHS;
        Shader<ID3D11DomainShader> bezierDS;
        Shader<ID3D11GeometryShader> bezierBorderGS;
        Shader<ID3D11PixelShader> bezierPS;

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
    };

    std::vector<ComPtr<ID3D11Buffer>> mVertexBuffers;
    std::vector<ComPtr<ID3D11Buffer>> mIndexBuffers;

    ComPtr<ID3D11ShaderResourceView> createShaderResourceView(const fs::path& texPath) const;

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

    ID3D11InputLayout* getBezierInputLayout() const
    {
        return mpBezierInputLayout.Get();
    }

    template <typename CB>
    void createCB(CB& cb)
    {
        static_assert(std::is_base_of<ICB, CB>::value, "CB must inherit from ICB");

        const size_t cbSize = sizeof(CB);
        const size_t iSize = sizeof(ICB);
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

    template <typename CB>
    void updateCB(const CB& cb)
    {
        static_assert(std::is_base_of<ICB, CB>::value, "CB must inherit from ICB");

        D3D11_MAPPED_SUBRESOURCE cbData;
        mpContext->Map(cb.buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbData);
        const char* baseAddress = reinterpret_cast<const char*>(&cb);
        memcpy(cbData.pData, baseAddress + sizeof(ICB), sizeof(CB) - sizeof(ICB));
        mpContext->Unmap(cb.buffer.Get(), 0);
    }
    void addRenderable(std::unique_ptr<IRenderable>&& renderable);
    void removeRenderable(Id id);
    IRenderable* getRenderable(Id id) const override;
    void createShaders();

private:
    void initCore();
    void buildVertexLayout();

    Shaders mShaders;

    ComPtr<ID3D11Device> mpDevice;
    ComPtr<IDXGISwapChain> mpSwapChain;
    ComPtr<ID3D11Texture2D> mpDepthStencilBuffer;
    ComPtr<ID3D11DeviceContext> mpContext;
    ComPtr<ID3D11RenderTargetView> mpRenderTargetView;
    ComPtr<ID3D11DepthStencilView> mpDepthStencilView;

    ComPtr<ID3D11InputLayout> mpDefaultInputLayout;
    ComPtr<ID3D11InputLayout> mpBezierInputLayout;

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

    IDXGIAdapter* selectedAdapter{};
    if (adapters.size() > 1)
    {
        selectedAdapter = adapters[1].Get(); // Second device is usually discrete GPU
    }
    else
    {
        selectedAdapter = adapters[0].Get();
    }

    DXGI_ADAPTER_DESC adapterDesc;
    if (SUCCEEDED(selectedAdapter->GetDesc(&adapterDesc)))
    {
        auto var = std::format(L"Selected GPU: {}", adapterDesc.Description).data();
        //WLOG(var);
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
    for (unsigned i{}; i < mRenderables.size(); ++i)
    {
        if (!mRenderables[i]->mShouldRebuildBuffers)
        {
            continue;
        }
        auto& pRenderable = mRenderables[i];
        // TODO: not yet implemented
        //if (mRenderables[i] == nullptr)
        //{
        //    continue;
        //}

        const std::vector<XMFLOAT3>& geometry = pRenderable->getGeometry();
        const std::vector<unsigned>& topology = pRenderable->getTopology();

        if (geometry.size() == 0)
        {
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

        mRenderables[i]->mShouldRebuildBuffers = false;
    }

    //mRebuildBuffers = false;
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
    // clang-format off
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
            mShaders.bezierVS.second->GetBufferPointer(), mShaders.bezierVS.second->GetBufferSize(), &mpBezierInputLayout));
    }
    // clang-format on
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

void DX11Renderer::addRenderable(std::unique_ptr<IRenderable>&& renderable)
{
    mRenderables.emplace_back(std::move(renderable));
    mVertexBuffers.emplace_back();
    mIndexBuffers.emplace_back();

    // mRebuildBuffers = true;
}

void DX11Renderer::removeRenderable(Id id)
{
    for (int i{}; i < mRenderables.size(); ++i)
    {
        if (mRenderables.at(i)->id == id)
        {
            mRenderables.erase(mRenderables.begin() + i);
            mVertexBuffers.erase(mVertexBuffers.begin() + i);
            mIndexBuffers.erase(mIndexBuffers.begin() + i);
        }
    }
}

IRenderable* DX11Renderer::getRenderable(Id id) const
{
    auto it =
        std::find_if(mRenderables.begin(), mRenderables.end(), [id](const auto& item) { return item->id == id; });

    if (it != mRenderables.end())
    {
        return it->get();
    }

    return nullptr;
}