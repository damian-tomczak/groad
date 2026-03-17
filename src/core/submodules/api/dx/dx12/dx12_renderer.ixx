module;

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include "mg.hpp"
#include "imgui_impl_dx12.h"

#include "utils.h"

export module dx12renderer;

import utils;
export import core.renderer;
import std;

import platform;

using namespace Microsoft::WRL;
using namespace DirectX;

export class DX12Renderer : public IRenderer
{
    static constexpr UINT FrameCount = 2;
    static constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    static constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // SRV heap layout: slot 0 = ImGui font, rest available for user textures
    static constexpr UINT SrvHeapSize = 64;

public:
    DX12Renderer(std::weak_ptr<IWindow> pWindow) : IRenderer{pWindow}
    {
    }

    ~DX12Renderer()
    {
        waitForGpu();

        clearRenderables();

        ImGui_ImplDX12_Shutdown();

        if (mFenceEvent != nullptr)
        {
            CloseHandle(mFenceEvent);
            mFenceEvent = nullptr;
        }
    }

    struct GlobalCB
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

    struct Shaders
    {
        using ShaderBlob = ComPtr<ID3DBlob>;

        ShaderBlob defaultVS;
        ShaderBlob defaultPS;

        ShaderBlob gridVS;
        ShaderBlob gridPS;

        ShaderBlob cursorVS;
        ShaderBlob cursorGS;
        ShaderBlob cursorPS;

        ShaderBlob bezierCurveVS;
        ShaderBlob bezierCurveHS;
        ShaderBlob bezierCurveDS;
        ShaderBlob bezierCurveBorderGS;
        ShaderBlob bezierCurvePS;

        ShaderBlob bezierPatchC0VS;
        ShaderBlob bezierPatchC0HS;
        ShaderBlob bezierPatchC0DS;
        ShaderBlob bezierPatchC0PS;

        ShaderBlob bezierPatchC2VS;
        ShaderBlob bezierPatchC2HS;
        ShaderBlob bezierPatchC2DS;
        ShaderBlob bezierPatchC2GS;
        ShaderBlob bezierPatchC2PS;

        ShaderBlob waterSurfaceVS;
        ShaderBlob waterSurfacePS;

        ShaderBlob wallsVS;
        ShaderBlob wallsPS;

        ShaderBlob duckVS;
        ShaderBlob duckPS;

        ShaderBlob gableBorderVS;
        ShaderBlob gableBorderPS;

        ShaderBlob gableVS;
        ShaderBlob gableHS;
        ShaderBlob gableDS;
        ShaderBlob gablePS;

        ShaderBlob billboardVS;
        ShaderBlob billboardPS;

        ShaderBlob blendVS;
        ShaderBlob blendPS;
    };

    enum class PSOType
    {
        DEFAULT_SOLID,
        DEFAULT_WIREFRAME,
        DEFAULT_LINE,
        GRID,
        CURSOR,
        BEZIER_CURVE,
        BEZIER_PATCH_C0,
        BEZIER_PATCH_C2,
        WATER_SURFACE,
        WALLS,
        DUCK,
        GABLE_BORDER,
        GABLE,
        BILLBOARD,
        BLEND,
        COUNT
    };

    void init(unsigned short gpuIndex) override;
    void onResize() override;
    void buildGeometryBuffers();

    void beginFrame();
    void endFrame(bool vsync);

    void clearRenderTarget(const float* clearColor);
    void clearDepthStencil();
    void setDefaultViewportAndScissor();

    void updateGlobalCB();

    void setPSO(PSOType type)
    {
        ASSERT(mpCommandList);
        auto it = mPSOs.find(type);
        if (it != mPSOs.end() && it->second)
        {
            mpCommandList->SetPipelineState(it->second.Get());
        }
    }

    void setSolidRaster(bool isCull = false)
    {
        setPSO(PSOType::DEFAULT_SOLID);
    }

    void setWireframeRaster(bool isCull = false)
    {
        setPSO(PSOType::DEFAULT_WIREFRAME);
    }

    const Shaders& getShaders() const
    {
        return mShaders;
    }

    ID3D12Device* getDevice() const
    {
        return mpDevice.Get();
    }

    ID3D12GraphicsCommandList* getCommandList() const
    {
        return mpCommandList.Get();
    }

    ID3D12CommandQueue* getCommandQueue() const
    {
        return mpCommandQueue.Get();
    }

    ID3D12DescriptorHeap* getSrvHeap() const
    {
        return mpCbvSrvUavHeap.Get();
    }

    const D3D12_VIEWPORT& getViewport() const
    {
        return mViewport;
    }

    void addRenderable(std::unique_ptr<IRenderable>&& renderable) override;
    void addRenderable(std::vector<std::unique_ptr<IRenderable>>&& renderables) override
    {
        ASSERT(false);
    }
    void removeRenderable(Id id) override;
    void removeRenderable(std::vector<Id> ids) override
    {
        ASSERT(false);
    }
    IRenderable* getRenderable(Id id) const override;

    void createShaders();

    std::vector<ComPtr<ID3D12Resource>> mVertexBuffers;
    std::vector<D3D12_VERTEX_BUFFER_VIEW> mVertexBufferViews;
    std::vector<ComPtr<ID3D12Resource>> mIndexBuffers;
    std::vector<D3D12_INDEX_BUFFER_VIEW> mIndexBufferViews;

    // Upload helpers for vertex/index data
    std::vector<ComPtr<ID3D12Resource>> mVertexUploadBuffers;
    std::vector<ComPtr<ID3D12Resource>> mIndexUploadBuffers;

private:
    void initCore(unsigned short gpuIndex);
    void createCommandObjects();
    void createSwapChain();
    void createDescriptorHeaps();
    void createRootSignature();
    void createPipelineStates();
    void createDepthStencilBuffer();
    void createGlobalCBResource();
    void waitForGpu();
    void moveToNextFrame();

    void transitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

    ComPtr<ID3D12PipelineState> createPSO(
        const char* debugName,
        ID3DBlob* vs, ID3DBlob* ps,
        ID3DBlob* hs, ID3DBlob* ds, ID3DBlob* gs,
        const D3D12_INPUT_ELEMENT_DESC* inputLayout, UINT inputLayoutCount,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE topoType,
        D3D12_FILL_MODE fillMode,
        bool enableBlend);

    Shaders mShaders;
    std::unordered_map<PSOType, ComPtr<ID3D12PipelineState>> mPSOs;

    ComPtr<ID3D12Device> mpDevice;
    ComPtr<IDXGIFactory4> mpDxgiFactory;

    ComPtr<ID3D12CommandQueue> mpCommandQueue;
    ComPtr<ID3D12CommandAllocator> mpCommandAllocators[FrameCount];
    ComPtr<ID3D12GraphicsCommandList> mpCommandList;

    ComPtr<IDXGISwapChain3> mpSwapChain;
    ComPtr<ID3D12Resource> mpRenderTargets[FrameCount];
    UINT mFrameIndex{};

    ComPtr<ID3D12DescriptorHeap> mpRtvHeap;
    ComPtr<ID3D12DescriptorHeap> mpDsvHeap;
    ComPtr<ID3D12DescriptorHeap> mpCbvSrvUavHeap;
    UINT mRtvDescriptorSize{};
    UINT mDsvDescriptorSize{};
    UINT mCbvSrvUavDescriptorSize{};

    ComPtr<ID3D12Resource> mpDepthStencilBuffer;

    ComPtr<ID3D12Fence> mpFence;
    UINT64 mFenceValues[FrameCount]{};
    HANDLE mFenceEvent{};

    ComPtr<ID3D12RootSignature> mpRootSignature;

    ComPtr<ID3D12Resource> mpGlobalCBUploadBuffer;
    UINT8* mpGlobalCBDataBegin{};

    D3D12_VIEWPORT mViewport{};
    D3D12_RECT mScissorRect{};
};

module :private;

static inline UINT AlignCB(UINT size)
{
    return (size + 255) & ~255;
}

static inline D3D12_RENDER_TARGET_BLEND_DESC createDefaultRenderTargetBlendDesc()
{
    return {
        .BlendEnable = FALSE,
        .LogicOpEnable = FALSE,
        .SrcBlend = D3D12_BLEND_ONE,
        .DestBlend = D3D12_BLEND_ZERO,
        .BlendOp = D3D12_BLEND_OP_ADD,
        .SrcBlendAlpha = D3D12_BLEND_ONE,
        .DestBlendAlpha = D3D12_BLEND_ZERO,
        .BlendOpAlpha = D3D12_BLEND_OP_ADD,
        .LogicOp = D3D12_LOGIC_OP_NOOP,
        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
    };
}

static inline D3D12_BLEND_DESC createBlendDesc(bool enableBlend)
{
    D3D12_BLEND_DESC blendDesc{
        .AlphaToCoverageEnable = FALSE,
        .IndependentBlendEnable = FALSE,
    };

    const auto defaultRtBlendDesc = createDefaultRenderTargetBlendDesc();
    for (auto& renderTargetBlendDesc : blendDesc.RenderTarget)
    {
        renderTargetBlendDesc = defaultRtBlendDesc;
    }

    if (enableBlend)
    {
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    }

    return blendDesc;
}

static inline D3D12_DEPTH_STENCILOP_DESC createDefaultDepthStencilOpDesc()
{
    return {
        .StencilFailOp = D3D12_STENCIL_OP_KEEP,
        .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
        .StencilPassOp = D3D12_STENCIL_OP_KEEP,
        .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS,
    };
}

static inline D3D12_DEPTH_STENCIL_DESC createDepthStencilDesc()
{
    const auto defaultStencilOp = createDefaultDepthStencilOpDesc();
    return {
        .DepthEnable = TRUE,
        .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc = D3D12_COMPARISON_FUNC_LESS,
        .StencilEnable = FALSE,
        .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
        .StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
        .FrontFace = defaultStencilOp,
        .BackFace = defaultStencilOp,
    };
}

void DX12Renderer::init(unsigned short gpuIndex)
{
    initCore(gpuIndex);
    createCommandObjects();
    createSwapChain();
    createDescriptorHeaps();
    createRootSignature();
    createShaders();
    createPipelineStates();
    createGlobalCBResource();
    onResize();

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = mpCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = mpCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();

    ImGui_ImplDX12_Init(
        mpDevice.Get(),
        FrameCount,
        BackBufferFormat,
        mpCbvSrvUavHeap.Get(),
        cpuHandle,
        gpuHandle);
}

void DX12Renderer::initCore(unsigned short gpuIndex)
{
    UINT dxgiFactoryFlags = 0;

#ifdef DEBUG
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    HR(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mpDxgiFactory)));

    std::vector<ComPtr<IDXGIAdapter1>> adapters;
    ComPtr<IDXGIAdapter1> pAdapter;
    for (UINT i = 0; mpDxgiFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        adapters.push_back(pAdapter);
        pAdapter.Reset();
    }

    if (adapters.size() == 0)
    {
        ERR("No GPU detected!");
    }

    if (gpuIndex >= adapters.size())
    {
        ERR(std::format("Following {} gpu index doesn't exist", gpuIndex));
    }

    IDXGIAdapter1* pSelectedAdapter = adapters[gpuIndex].Get();

    DXGI_ADAPTER_DESC1 adapterDesc;
    HR(pSelectedAdapter->GetDesc1(&adapterDesc));

    WLOG(std::format(L"Selected GPU (DX12): {}", adapterDesc.Description).data());

    HR(D3D12CreateDevice(pSelectedAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mpDevice)));
}

void DX12Renderer::createCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc{
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
        .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
    };
    HR(mpDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mpCommandQueue)));

    for (UINT i = 0; i < FrameCount; ++i)
    {
        HR(mpDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mpCommandAllocators[i])));
    }

    HR(mpDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        mpCommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&mpCommandList)));
    HR(mpCommandList->Close());

    HR(mpDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mpFence)));
    mFenceValues[mFrameIndex] = 1;

    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (mFenceEvent == nullptr)
    {
        ERR("Failed to create fence event");
    }
}

void DX12Renderer::createSwapChain()
{
    const auto pWindow = dynamic_cast<Win32Window*>(mpWindow.lock().get());
    ASSERT(pWindow != nullptr);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{
        .Width = static_cast<UINT>(pWindow->getWidth()),
        .Height = static_cast<UINT>(pWindow->getHeight()),
        .Format = BackBufferFormat,
        .SampleDesc{.Count = 1, .Quality = 0},
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = FrameCount,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    };

    ComPtr<IDXGISwapChain1> swapChain;
    HR(mpDxgiFactory->CreateSwapChainForHwnd(
        mpCommandQueue.Get(),
        pWindow->getHwnd(),
        &swapChainDesc,
        nullptr, nullptr,
        &swapChain));

    HR(mpDxgiFactory->MakeWindowAssociation(pWindow->getHwnd(), DXGI_MWA_NO_ALT_ENTER));
    HR(swapChain.As(&mpSwapChain));

    mFrameIndex = mpSwapChain->GetCurrentBackBufferIndex();
}

void DX12Renderer::createDescriptorHeaps()
{
    // RTV heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = FrameCount,
        };
        HR(mpDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mpRtvHeap)));
        mRtvDescriptorSize = mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // DSV heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            .NumDescriptors = 1,
        };
        HR(mpDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mpDsvHeap)));
        mDsvDescriptorSize = mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    // CBV/SRV/UAV heap (shader-visible, for ImGui fonts + user textures)
    {
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = SrvHeapSize,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        };
        HR(mpDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mpCbvSrvUavHeap)));
        mCbvSrvUavDescriptorSize = mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
}

void DX12Renderer::createRootSignature()
{
    D3D12_DESCRIPTOR_RANGE srvRange{};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 2;
    srvRange.BaseShaderRegister = 0;
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // Root parameter 0: CBV at register(b0) - global constant buffer
    // Root parameter 1: CBV at register(b2) - extra tessellation factors buffer
    // Root parameter 2: SRV descriptor table at register(t0) - textured pixel shaders
    D3D12_ROOT_PARAMETER rootParams[3]{};
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[1].Descriptor.ShaderRegister = 2;
    rootParams[1].Descriptor.RegisterSpace = 0;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC staticSampler{};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.MipLODBias = 0.0f;
    staticSampler.MaxAnisotropy = 1;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    staticSampler.MinLOD = 0.0f;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0;
    staticSampler.RegisterSpace = 0;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc{
        .NumParameters = _countof(rootParams),
        .pParameters = rootParams,
        .NumStaticSamplers = 1,
        .pStaticSamplers = &staticSampler,
        .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
    };

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    HRESULT hrResult = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hrResult))
    {
        if (error)
        {
            ERR(static_cast<const char*>(error->GetBufferPointer()));
        }
        HR(hrResult);
    }

    HR(mpDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mpRootSignature)));
}

ComPtr<ID3D12PipelineState> DX12Renderer::createPSO(
    const char* debugName,
    ID3DBlob* vs, ID3DBlob* ps,
    ID3DBlob* hs, ID3DBlob* ds, ID3DBlob* gs,
    const D3D12_INPUT_ELEMENT_DESC* inputLayout, UINT inputLayoutCount,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE topoType,
    D3D12_FILL_MODE fillMode,
    bool enableBlend)
{
    const bool usesLineOrPointTopology = topoType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE
        || topoType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    if (fillMode == D3D12_FILL_MODE_WIREFRAME && usesLineOrPointTopology)
    {
        WARN(std::format(
            "DX12 PSO requested wireframe fill for topology type {}; using solid fill instead.",
            static_cast<int>(topoType)));
        fillMode = D3D12_FILL_MODE_SOLID;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = {inputLayout, inputLayoutCount};
    psoDesc.pRootSignature = mpRootSignature.Get();

    if (vs) psoDesc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
    if (ps) psoDesc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
    if (hs) psoDesc.HS = {hs->GetBufferPointer(), hs->GetBufferSize()};
    if (ds) psoDesc.DS = {ds->GetBufferPointer(), ds->GetBufferSize()};
    if (gs) psoDesc.GS = {gs->GetBufferPointer(), gs->GetBufferSize()};

    psoDesc.RasterizerState = {
        .FillMode = fillMode,
        .CullMode = D3D12_CULL_MODE_NONE,
        .FrontCounterClockwise = FALSE,
        .DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
        .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
        .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        .DepthClipEnable = TRUE,
        .MultisampleEnable = FALSE,
        .AntialiasedLineEnable = FALSE,
        .ForcedSampleCount = 0,
        .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
    };

    psoDesc.BlendState = createBlendDesc(enableBlend);
    psoDesc.DepthStencilState = createDepthStencilDesc();

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = topoType;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = BackBufferFormat;
    psoDesc.DSVFormat = DepthStencilFormat;
    psoDesc.SampleDesc = {.Count = 1, .Quality = 0};

    ComPtr<ID3D12PipelineState> pso;
    const HRESULT hr = mpDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    if (FAILED(hr))
    {
        ERR(std::format(
            "CreateGraphicsPipelineState failed for '{}' (topology={}, fillMode={}, inputLayoutCount={}, shaders: vs={}, ps={}, hs={}, ds={}, gs={}) {}({})",
            debugName,
            static_cast<int>(topoType),
            static_cast<int>(fillMode),
            inputLayoutCount,
            vs != nullptr,
            ps != nullptr,
            hs != nullptr,
            ds != nullptr,
            gs != nullptr,
            std::system_category().message(hr),
            hr));
    }
    return pso;
}

void DX12Renderer::createPipelineStates()
{
    // Default input layout (position only)
    D3D12_INPUT_ELEMENT_DESC defaultLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    // Position + normal input layout
    D3D12_INPUT_ELEMENT_DESC positionNormalLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 3 * sizeof(float),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    // Position + normal + UV input layout
    D3D12_INPUT_ELEMENT_DESC texturedLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 3 * sizeof(float),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 6 * sizeof(float),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    constexpr D3D12_INPUT_ELEMENT_DESC* noInputLayout = nullptr;
    constexpr UINT noInputLayoutCount = 0;

    // Bezier curve input layout (4 control points)
    D3D12_INPUT_ELEMENT_DESC bezierCurveLayout[] = {
        {"CONTROL_POINT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"CONTROL_POINT", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 3 * sizeof(float),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"CONTROL_POINT", 2, DXGI_FORMAT_R32G32B32_FLOAT, 0, 6 * sizeof(float),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"CONTROL_POINT", 3, DXGI_FORMAT_R32G32B32_FLOAT, 0, 9 * sizeof(float),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    // Bezier patch input layout (16 control points)
    static constexpr size_t sizeOfR32G32B32 = 3 * sizeof(float);
    D3D12_INPUT_ELEMENT_DESC bezierPatchLayout[16];
    for (UINT i = 0; i < 16; ++i)
    {
        bezierPatchLayout[i] = {"CONTROL_POINT", i, DXGI_FORMAT_R32G32B32_FLOAT, 0, i * static_cast<UINT>(sizeOfR32G32B32), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    }

    auto tryCreatePSO = [this](
        PSOType type,
        const char* debugName,
        ID3DBlob* vs, ID3DBlob* ps,
        ID3DBlob* hs, ID3DBlob* ds, ID3DBlob* gs,
        const D3D12_INPUT_ELEMENT_DESC* layout, UINT layoutCount,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE topoType,
        D3D12_FILL_MODE fillMode,
        bool enableBlend)
    {
        if (vs && ps)
        {
            mPSOs[type] = createPSO(debugName, vs, ps, hs, ds, gs, layout, layoutCount, topoType, fillMode, enableBlend);
        }
    };

    // Default solid
    tryCreatePSO(PSOType::DEFAULT_SOLID,
        "DEFAULT_SOLID",
        mShaders.defaultVS.Get(), mShaders.defaultPS.Get(), nullptr, nullptr, nullptr,
        defaultLayout, _countof(defaultLayout),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D12_FILL_MODE_SOLID, true);

    // Default wireframe for triangle meshes
    tryCreatePSO(PSOType::DEFAULT_WIREFRAME,
        "DEFAULT_WIREFRAME",
        mShaders.defaultVS.Get(), mShaders.defaultPS.Get(), nullptr, nullptr, nullptr,
        defaultLayout, _countof(defaultLayout),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D12_FILL_MODE_WIREFRAME, true);

    // Default line-list rendering
    tryCreatePSO(PSOType::DEFAULT_LINE,
        "DEFAULT_LINE",
        mShaders.defaultVS.Get(), mShaders.defaultPS.Get(), nullptr, nullptr, nullptr,
        defaultLayout, _countof(defaultLayout),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE, D3D12_FILL_MODE_SOLID, true);

    // Grid
    tryCreatePSO(PSOType::GRID,
        "GRID",
        mShaders.gridVS.Get(), mShaders.gridPS.Get(), nullptr, nullptr, nullptr,
        noInputLayout, noInputLayoutCount,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D12_FILL_MODE_SOLID, true);

    // Cursor (with geometry shader)
    tryCreatePSO(PSOType::CURSOR,
        "CURSOR",
        mShaders.cursorVS.Get(), mShaders.cursorPS.Get(), nullptr, nullptr, mShaders.cursorGS.Get(),
        noInputLayout, noInputLayoutCount,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE, D3D12_FILL_MODE_SOLID, true);

    // Bezier curve (with tessellation)
    tryCreatePSO(PSOType::BEZIER_CURVE,
        "BEZIER_CURVE",
        mShaders.bezierCurveVS.Get(), mShaders.bezierCurvePS.Get(),
        mShaders.bezierCurveHS.Get(), mShaders.bezierCurveDS.Get(), nullptr,
        bezierCurveLayout, _countof(bezierCurveLayout),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH, D3D12_FILL_MODE_SOLID, true);

    // Bezier patch C0
    tryCreatePSO(PSOType::BEZIER_PATCH_C0,
        "BEZIER_PATCH_C0",
        mShaders.bezierPatchC0VS.Get(), mShaders.bezierPatchC0PS.Get(),
        mShaders.bezierPatchC0HS.Get(), mShaders.bezierPatchC0DS.Get(), nullptr,
        bezierPatchLayout, 16,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH, D3D12_FILL_MODE_SOLID, true);

    // Bezier patch C2 (with geometry shader)
    tryCreatePSO(PSOType::BEZIER_PATCH_C2,
        "BEZIER_PATCH_C2",
        mShaders.bezierPatchC2VS.Get(), mShaders.bezierPatchC2PS.Get(),
        mShaders.bezierPatchC2HS.Get(), mShaders.bezierPatchC2DS.Get(), nullptr,
        bezierPatchLayout, 16,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH, D3D12_FILL_MODE_SOLID, true);

    // Water surface
    tryCreatePSO(PSOType::WATER_SURFACE,
        "WATER_SURFACE",
        mShaders.waterSurfaceVS.Get(), mShaders.waterSurfacePS.Get(), nullptr, nullptr, nullptr,
        noInputLayout, noInputLayoutCount,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D12_FILL_MODE_SOLID, true);

    // Walls
    tryCreatePSO(PSOType::WALLS,
        "WALLS",
        mShaders.wallsVS.Get(), mShaders.wallsPS.Get(), nullptr, nullptr, nullptr,
        positionNormalLayout, _countof(positionNormalLayout),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D12_FILL_MODE_SOLID, true);

    // Duck
    tryCreatePSO(PSOType::DUCK,
        "DUCK",
        mShaders.duckVS.Get(), mShaders.duckPS.Get(), nullptr, nullptr, nullptr,
        texturedLayout, _countof(texturedLayout),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D12_FILL_MODE_SOLID, true);

    // Gable border
    tryCreatePSO(PSOType::GABLE_BORDER,
        "GABLE_BORDER",
        mShaders.gableBorderVS.Get(), mShaders.gableBorderPS.Get(), nullptr, nullptr, nullptr,
        defaultLayout, _countof(defaultLayout),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE, D3D12_FILL_MODE_SOLID, true);

    // Gable (with tessellation)
    tryCreatePSO(PSOType::GABLE,
        "GABLE",
        mShaders.gableVS.Get(), mShaders.gablePS.Get(),
        mShaders.gableHS.Get(), mShaders.gableDS.Get(), nullptr,
        defaultLayout, _countof(defaultLayout),
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH, D3D12_FILL_MODE_SOLID, true);

    // Billboard
    tryCreatePSO(PSOType::BILLBOARD,
        "BILLBOARD",
        mShaders.billboardVS.Get(), mShaders.billboardPS.Get(), nullptr, nullptr, nullptr,
        noInputLayout, noInputLayoutCount,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D12_FILL_MODE_SOLID, true);

    // Blend
    tryCreatePSO(PSOType::BLEND,
        "BLEND",
        mShaders.blendVS.Get(), mShaders.blendPS.Get(), nullptr, nullptr, nullptr,
        noInputLayout, noInputLayoutCount,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D12_FILL_MODE_SOLID, true);
}

void DX12Renderer::createGlobalCBResource()
{
    const UINT cbSize = AlignCB(sizeof(GlobalCB));

    D3D12_HEAP_PROPERTIES heapProps{
        .Type = D3D12_HEAP_TYPE_UPLOAD,
    };

    D3D12_RESOURCE_DESC resourceDesc{
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Width = cbSize,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .SampleDesc{.Count = 1},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
    };

    HR(mpDevice->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE,
        &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&mpGlobalCBUploadBuffer)));

    D3D12_RANGE readRange{0, 0};
    HR(mpGlobalCBUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mpGlobalCBDataBegin)));
}

void DX12Renderer::updateGlobalCB()
{
    memcpy(mpGlobalCBDataBegin, &mGlobalCB, sizeof(GlobalCB));
}

void DX12Renderer::onResize()
{
    ASSERT(mpDevice);
    ASSERT(mpSwapChain);

    waitForGpu();

    for (UINT i = 0; i < FrameCount; ++i)
    {
        mpRenderTargets[i].Reset();
        mFenceValues[i] = mFenceValues[mFrameIndex];
    }
    mpDepthStencilBuffer.Reset();

    const auto pWindow = mpWindow.lock().get();
    ASSERT(pWindow);

    UINT width = static_cast<UINT>(pWindow->getWidth());
    UINT height = static_cast<UINT>(pWindow->getHeight());

    HR(mpSwapChain->ResizeBuffers(FrameCount, width, height, BackBufferFormat, 0));
    mFrameIndex = mpSwapChain->GetCurrentBackBufferIndex();

    // Recreate RTVs
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mpRtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < FrameCount; ++i)
        {
            HR(mpSwapChain->GetBuffer(i, IID_PPV_ARGS(&mpRenderTargets[i])));
            mpDevice->CreateRenderTargetView(mpRenderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += mRtvDescriptorSize;
        }
    }

    createDepthStencilBuffer();

    mViewport = {
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = static_cast<float>(width),
        .Height = static_cast<float>(height),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    };

    mScissorRect = {
        .left = 0,
        .top = 0,
        .right = static_cast<LONG>(width),
        .bottom = static_cast<LONG>(height),
    };
}

void DX12Renderer::createDepthStencilBuffer()
{
    const auto pWindow = mpWindow.lock().get();
    ASSERT(pWindow);

    D3D12_HEAP_PROPERTIES heapProps{
        .Type = D3D12_HEAP_TYPE_DEFAULT,
    };

    D3D12_RESOURCE_DESC depthDesc{
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Width = static_cast<UINT64>(pWindow->getWidth()),
        .Height = static_cast<UINT>(pWindow->getHeight()),
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DepthStencilFormat,
        .SampleDesc{.Count = 1, .Quality = 0},
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
    };

    D3D12_CLEAR_VALUE clearValue{
        .Format = DepthStencilFormat,
        .DepthStencil{.Depth = 1.0f, .Stencil = 0},
    };

    HR(mpDevice->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE,
        &depthDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue, IID_PPV_ARGS(&mpDepthStencilBuffer)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{
        .Format = DepthStencilFormat,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
    };

    mpDevice->CreateDepthStencilView(mpDepthStencilBuffer.Get(), &dsvDesc,
        mpDsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void DX12Renderer::transitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_BARRIER barrier{
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition{
            .pResource = resource,
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = before,
            .StateAfter = after,
        },
    };
    mpCommandList->ResourceBarrier(1, &barrier);
}

void DX12Renderer::beginFrame()
{
    HR(mpCommandAllocators[mFrameIndex]->Reset());
    HR(mpCommandList->Reset(mpCommandAllocators[mFrameIndex].Get(), nullptr));

    transitionResource(mpRenderTargets[mFrameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mpRtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += mFrameIndex * mRtvDescriptorSize;

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mpDsvHeap->GetCPUDescriptorHandleForHeapStart();

    mpCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    mpCommandList->RSSetViewports(1, &mViewport);
    mpCommandList->RSSetScissorRects(1, &mScissorRect);

    ID3D12DescriptorHeap* heaps[] = {mpCbvSrvUavHeap.Get()};
    mpCommandList->SetDescriptorHeaps(_countof(heaps), heaps);

    mpCommandList->SetGraphicsRootSignature(mpRootSignature.Get());
    mpCommandList->SetGraphicsRootConstantBufferView(0, mpGlobalCBUploadBuffer->GetGPUVirtualAddress());
}

void DX12Renderer::clearRenderTarget(const float* clearColor)
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mpRtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += mFrameIndex * mRtvDescriptorSize;
    mpCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void DX12Renderer::clearDepthStencil()
{
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mpDsvHeap->GetCPUDescriptorHandleForHeapStart();
    mpCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
}

void DX12Renderer::setDefaultViewportAndScissor()
{
    mpCommandList->RSSetViewports(1, &mViewport);
    mpCommandList->RSSetScissorRects(1, &mScissorRect);
}

void DX12Renderer::endFrame(bool vsync)
{
    transitionResource(mpRenderTargets[mFrameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    HR(mpCommandList->Close());

    ID3D12CommandList* ppCommandLists[] = {mpCommandList.Get()};
    mpCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    HR(mpSwapChain->Present(vsync ? 1 : 0, 0));

    moveToNextFrame();
}

void DX12Renderer::waitForGpu()
{
    if (mpCommandQueue == nullptr || mpFence == nullptr || mFenceEvent == nullptr)
    {
        return;
    }

    HR(mpCommandQueue->Signal(mpFence.Get(), mFenceValues[mFrameIndex]));
    HR(mpFence->SetEventOnCompletion(mFenceValues[mFrameIndex], mFenceEvent));
    WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);
    ++mFenceValues[mFrameIndex];
}

void DX12Renderer::moveToNextFrame()
{
    const UINT64 currentFenceValue = mFenceValues[mFrameIndex];
    HR(mpCommandQueue->Signal(mpFence.Get(), currentFenceValue));

    mFrameIndex = mpSwapChain->GetCurrentBackBufferIndex();

    if (mpFence->GetCompletedValue() < mFenceValues[mFrameIndex])
    {
        HR(mpFence->SetEventOnCompletion(mFenceValues[mFrameIndex], mFenceEvent));
        WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);
    }

    mFenceValues[mFrameIndex] = currentFenceValue + 1;
}

void DX12Renderer::buildGeometryBuffers()
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
            ASSERT(false);
            continue;
        }

        mVertexBuffers[i].Reset();
        mVertexUploadBuffers[i].Reset();
        mIndexBuffers[i].Reset();
        mIndexUploadBuffers[i].Reset();

        const UINT vbSize = static_cast<UINT>(sizeof(XMFLOAT3) * geometry.size());

        // Create vertex buffer (default heap)
        D3D12_HEAP_PROPERTIES defaultHeap{.Type = D3D12_HEAP_TYPE_DEFAULT};
        D3D12_HEAP_PROPERTIES uploadHeap{.Type = D3D12_HEAP_TYPE_UPLOAD};

        D3D12_RESOURCE_DESC bufDesc{
            .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Width = vbSize,
            .Height = 1,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .SampleDesc{.Count = 1},
            .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        };

        HR(mpDevice->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE,
            &bufDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&mVertexBuffers[i])));

        HR(mpDevice->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE,
            &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mVertexUploadBuffers[i])));

        // Copy vertex data via upload buffer
        UINT8* pData = nullptr;
        D3D12_RANGE readRange{0, 0};
        HR(mVertexUploadBuffers[i]->Map(0, &readRange, reinterpret_cast<void**>(&pData)));
        memcpy(pData, geometry.data(), vbSize);
        mVertexUploadBuffers[i]->Unmap(0, nullptr);

        transitionResource(mVertexBuffers[i].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
        mpCommandList->CopyResource(mVertexBuffers[i].Get(), mVertexUploadBuffers[i].Get());
        transitionResource(mVertexBuffers[i].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        mVertexBufferViews[i] = {
            .BufferLocation = mVertexBuffers[i]->GetGPUVirtualAddress(),
            .SizeInBytes = vbSize,
            .StrideInBytes = pRenderable->getStride(),
        };

        if (topology.size() > 0)
        {
            const UINT ibSize = static_cast<UINT>(sizeof(unsigned) * topology.size());

            bufDesc.Width = ibSize;

            HR(mpDevice->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE,
                &bufDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&mIndexBuffers[i])));

            HR(mpDevice->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE,
                &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mIndexUploadBuffers[i])));

            HR(mIndexUploadBuffers[i]->Map(0, &readRange, reinterpret_cast<void**>(&pData)));
            memcpy(pData, topology.data(), ibSize);
            mIndexUploadBuffers[i]->Unmap(0, nullptr);

            transitionResource(mIndexBuffers[i].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
            mpCommandList->CopyResource(mIndexBuffers[i].Get(), mIndexUploadBuffers[i].Get());
            transitionResource(mIndexBuffers[i].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

            mIndexBufferViews[i] = {
                .BufferLocation = mIndexBuffers[i]->GetGPUVirtualAddress(),
                .SizeInBytes = ibSize,
                .Format = DXGI_FORMAT_R32_UINT,
            };
        }

        pRenderable->mShouldRebuildBuffers = false;
    }
}

void DX12Renderer::createShaders()
{
    bool isSuccess = true;

    static bool firstCall = true;

    auto compileShader = [&isSuccess](const fs::path shaderPath, const std::string_view shaderModel,
                             ComPtr<ID3DBlob>& shaderBlob)
    {
        shaderBlob.Reset();

        static constexpr std::string_view shaderEntryPoint{"main"};

#ifndef DEBUG
#error no impl
#endif

        const fs::path fullShaderPath = SHADERS_DIR / shaderPath;

        if (!fs::exists(fullShaderPath))
        {
            std::string errMsg = "Shaders compiler error: following path doesn't exist ";
            errMsg.append((fs::current_path() / fullShaderPath).string());
            ERR(errMsg.c_str());
        }

        ComPtr<ID3DBlob> compilationMsgs;
        DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef DEBUG
        shaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
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

        if (shaderBlob == nullptr)
        {
            isSuccess = false;
        }
    };

#define DEFAULT_SHADER_VER "_5_0"

#define DX12_COMPILE_VS(shaderName, shaderVar) compileShader(shaderName, "vs" DEFAULT_SHADER_VER, shaderVar);
#define DX12_COMPILE_PS(shaderName, shaderVar) compileShader(shaderName, "ps" DEFAULT_SHADER_VER, shaderVar);
#define DX12_COMPILE_HS(shaderName, shaderVar) compileShader(shaderName, "hs" DEFAULT_SHADER_VER, shaderVar);
#define DX12_COMPILE_DS(shaderName, shaderVar) compileShader(shaderName, "ds" DEFAULT_SHADER_VER, shaderVar);
#define DX12_COMPILE_GS(shaderName, shaderVar) compileShader(shaderName, "gs" DEFAULT_SHADER_VER, shaderVar);

    // Default renderable
    DX12_COMPILE_VS("default_renderable/default_renderable_vs.hlsl", mShaders.defaultVS);
    DX12_COMPILE_PS("default_renderable/default_renderable_ps.hlsl", mShaders.defaultPS);

    // Grid
    DX12_COMPILE_VS("surfaces/infinite_grid/infinite_grid_vs.hlsl", mShaders.gridVS);
    DX12_COMPILE_PS("surfaces/infinite_grid/infinite_grid_ps.hlsl", mShaders.gridPS);

    // Cursor
    DX12_COMPILE_VS("cursor/cursor_vs.hlsl", mShaders.cursorVS);
    DX12_COMPILE_GS("cursor/cursor_gs.hlsl", mShaders.cursorGS);
    DX12_COMPILE_PS("cursor/cursor_ps.hlsl", mShaders.cursorPS);

    // Bezier curve
    DX12_COMPILE_VS("beziers/bezier_curve/bezier_vs.hlsl", mShaders.bezierCurveVS);
    DX12_COMPILE_HS("beziers/bezier_curve/bezier_hs.hlsl", mShaders.bezierCurveHS);
    DX12_COMPILE_DS("beziers/bezier_curve/bezier_ds.hlsl", mShaders.bezierCurveDS);
    DX12_COMPILE_GS("beziers/bezier_curve/bezier_border_gs.hlsl", mShaders.bezierCurveBorderGS);
    DX12_COMPILE_PS("beziers/bezier_curve/bezier_ps.hlsl", mShaders.bezierCurvePS);

    // Bezier patch C0
    DX12_COMPILE_VS("beziers/bezier_patch_c0/bezier_patch_c0_vs.hlsl", mShaders.bezierPatchC0VS);
    DX12_COMPILE_HS("beziers/bezier_patch_c0/bezier_patch_c0_hs.hlsl", mShaders.bezierPatchC0HS);
    DX12_COMPILE_DS("beziers/bezier_patch_c0/bezier_patch_c0_ds.hlsl", mShaders.bezierPatchC0DS);
    DX12_COMPILE_PS("beziers/bezier_patch_c0/bezier_patch_c0_ps.hlsl", mShaders.bezierPatchC0PS);

    // Bezier patch C2
    DX12_COMPILE_VS("beziers/bezier_patch_c2/bezier_patch_c2_vs.hlsl", mShaders.bezierPatchC2VS);
    DX12_COMPILE_HS("beziers/bezier_patch_c2/bezier_patch_c2_hs.hlsl", mShaders.bezierPatchC2HS);
    DX12_COMPILE_DS("beziers/bezier_patch_c2/bezier_patch_c2_ds.hlsl", mShaders.bezierPatchC2DS);
    DX12_COMPILE_GS("beziers/bezier_patch_c2/bezier_patch_c2_border_gs.hlsl", mShaders.bezierPatchC2GS);
    DX12_COMPILE_PS("beziers/bezier_patch_c2/bezier_patch_c2_ps.hlsl", mShaders.bezierPatchC2PS);

    // Water surface
    DX12_COMPILE_VS("surfaces/water_surface/water_surface_vs.hlsl", mShaders.waterSurfaceVS);
    DX12_COMPILE_PS("surfaces/water_surface/water_surface_ps.hlsl", mShaders.waterSurfacePS);

    // Walls
    DX12_COMPILE_VS("walls/walls_vs.hlsl", mShaders.wallsVS);
    DX12_COMPILE_PS("walls/walls_ps.hlsl", mShaders.wallsPS);

    // Duck
    DX12_COMPILE_VS("duck/duck_vs.hlsl", mShaders.duckVS);
    DX12_COMPILE_PS("duck/duck_ps.hlsl", mShaders.duckPS);

    // Gable
    DX12_COMPILE_VS("gable/gable_border_vs.hlsl", mShaders.gableBorderVS);
    DX12_COMPILE_PS("gable/gable_border_ps.hlsl", mShaders.gableBorderPS);
    DX12_COMPILE_VS("gable/gable_vs.hlsl", mShaders.gableVS);
    DX12_COMPILE_HS("gable/gable_hs.hlsl", mShaders.gableHS);
    DX12_COMPILE_DS("gable/gable_ds.hlsl", mShaders.gableDS);
    DX12_COMPILE_PS("gable/gable_ps.hlsl", mShaders.gablePS);

    // Billboard
    DX12_COMPILE_VS("billboard/billboard_vs.hlsl", mShaders.billboardVS);
    DX12_COMPILE_PS("billboard/billboard_ps.hlsl", mShaders.billboardPS);

    // Blend
    DX12_COMPILE_VS("blend/blend_vs.hlsl", mShaders.blendVS);
    DX12_COMPILE_PS("blend/blend_ps.hlsl", mShaders.blendPS);

#undef DEFAULT_SHADER_VER
#undef DX12_COMPILE_VS
#undef DX12_COMPILE_PS
#undef DX12_COMPILE_HS
#undef DX12_COMPILE_DS
#undef DX12_COMPILE_GS

    firstCall = false;

    if (isSuccess)
    {
        LOG("Shader compilaton SUCCESS (DX12)");
    }
    else
    {
        ERR_NOTERMINATE("Shader compilation ERROR (DX12)");
    }
}

void DX12Renderer::addRenderable(std::unique_ptr<IRenderable>&& pRenderable)
{
    const Id renderableId = pRenderable->getId();
    ASSERT((pRenderable != nullptr) && (renderableId != invalidId));

    if (static_cast<size_t>(renderableId) == mRenderablePtrs.size())
    {
        mRenderablePtrs.emplace_back(std::move(pRenderable));
        mVertexBuffers.emplace_back();
        mVertexBufferViews.emplace_back();
        mVertexUploadBuffers.emplace_back();
        mIndexBuffers.emplace_back();
        mIndexBufferViews.emplace_back();
        mIndexUploadBuffers.emplace_back();
    }
    else
    {
        static constexpr size_t reasonablyEmptyAmountPredecessors = 500;
        ASSERT(!((renderableId - mRenderablePtrs.size()) > reasonablyEmptyAmountPredecessors));

        mRenderablePtrs.resize(renderableId + 1);
        mRenderablePtrs[renderableId] = std::move(pRenderable);
        mVertexBuffers.resize(renderableId + 1, nullptr);
        mVertexBufferViews.resize(renderableId + 1);
        mVertexUploadBuffers.resize(renderableId + 1, nullptr);
        mIndexBuffers.resize(renderableId + 1, nullptr);
        mIndexBufferViews.resize(renderableId + 1);
        mIndexUploadBuffers.resize(renderableId + 1, nullptr);
    }

    ASSERT(mRenderablePtrs[renderableId] != nullptr);
    mRenderablePtrs[renderableId]->regenerateData();
}

void DX12Renderer::removeRenderable(Id id)
{
    if (id == invalidId)
    {
        ASSERT(false);
        return;
    }

    ASSERT(mRenderablePtrs.size() > static_cast<size_t>(id));

    (mRenderablePtrs.begin() + id)->reset();
    mVertexBuffers[id].Reset();
    mVertexBufferViews[id] = {};
    mVertexUploadBuffers[id].Reset();
    mIndexBuffers[id].Reset();
    mIndexBufferViews[id] = {};
    mIndexUploadBuffers[id].Reset();

    IIdentifiable::mFreeIds.push(id);
}

IRenderable* DX12Renderer::getRenderable(Id id) const
{
    if ((id == invalidId) || (static_cast<size_t>(id) > mRenderablePtrs.size()))
    {
        ASSERT(false);
        return nullptr;
    }

    return mRenderablePtrs[id].get();
}
