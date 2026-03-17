module;

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <DirectXMath.h>

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

public:
    DX12Renderer(std::weak_ptr<IWindow> pWindow) : IRenderer{pWindow}
    {
    }

    ~DX12Renderer()
    {
        waitForGpu();

        clearRenderables();

        if (mFenceEvent != nullptr)
        {
            CloseHandle(mFenceEvent);
            mFenceEvent = nullptr;
        }
    }

    void init(unsigned short gpuIndex) override;
    void onResize() override;

    void addRenderable(std::unique_ptr<IRenderable>&& renderable) override;
    void addRenderable(std::vector<std::unique_ptr<IRenderable>>&& renderables) override;
    void removeRenderable(Id id) override;
    void removeRenderable(std::vector<Id> ids) override;
    IRenderable* getRenderable(Id id) const override;

    void buildGeometryBuffers();

    ID3D12Device* getDevice() const { return mpDevice.Get(); }
    ID3D12GraphicsCommandList* getCommandList() const { return mpCommandList.Get(); }
    ID3D12CommandQueue* getCommandQueue() const { return mpCommandQueue.Get(); }
    IDXGISwapChain3* getSwapChain() const { return mpSwapChain.Get(); }

    UINT getFrameIndex() const { return mFrameIndex; }

    ID3D12RootSignature* getRootSignature() const { return mpRootSignature.Get(); }

    D3D12_VIEWPORT getViewport() const { return mViewport; }
    D3D12_RECT getScissorRect() const { return mScissorRect; }

    void beginFrame(float clearR, float clearG, float clearB, float clearA);
    void endFrame(bool vsync);

    struct Shaders
    {
        ComPtr<ID3DBlob> defaultVS;
        ComPtr<ID3DBlob> defaultPS;
    };

    void createShaders();

    const Shaders& getShaders() const { return mShaders; }

    ID3D12PipelineState* getDefaultPSO() const { return mpDefaultPSO.Get(); }

    std::vector<ComPtr<ID3D12Resource>> mVertexBuffers;
    std::vector<ComPtr<ID3D12Resource>> mIndexBuffers;
    std::vector<D3D12_VERTEX_BUFFER_VIEW> mVertexBufferViews;
    std::vector<D3D12_INDEX_BUFFER_VIEW> mIndexBufferViews;

private:
    void initCore(unsigned short gpuIndex);
    void createCommandObjects();
    void createSwapChain();
    void createDescriptorHeaps();
    void createRenderTargetViews();
    void createDepthStencilBuffer();
    void createRootSignatureAndPSO();

    void waitForGpu();
    void moveToNextFrame();

    // Core DX12 objects
    ComPtr<IDXGIFactory4> mpDxgiFactory;
    ComPtr<ID3D12Device> mpDevice;
    ComPtr<ID3D12CommandQueue> mpCommandQueue;
    ComPtr<IDXGISwapChain3> mpSwapChain;

    // Command list resources
    ComPtr<ID3D12CommandAllocator> mpCommandAllocators[FrameCount];
    ComPtr<ID3D12GraphicsCommandList> mpCommandList;

    // Render targets
    ComPtr<ID3D12Resource> mpRenderTargets[FrameCount];
    ComPtr<ID3D12DescriptorHeap> mpRtvHeap;
    UINT mRtvDescriptorSize{};

    // Depth stencil
    ComPtr<ID3D12Resource> mpDepthStencilBuffer;
    ComPtr<ID3D12DescriptorHeap> mpDsvHeap;

    // CBV/SRV/UAV heap
    ComPtr<ID3D12DescriptorHeap> mpCbvSrvUavHeap;

    // Synchronization
    ComPtr<ID3D12Fence> mpFence;
    UINT64 mFenceValues[FrameCount]{};
    HANDLE mFenceEvent{};
    UINT mFrameIndex{};

    // Root signature and pipeline state
    ComPtr<ID3D12RootSignature> mpRootSignature;
    ComPtr<ID3D12PipelineState> mpDefaultPSO;

    // Viewport
    D3D12_VIEWPORT mViewport{};
    D3D12_RECT mScissorRect{};

    Shaders mShaders;
};

module :private;

void DX12Renderer::init(unsigned short gpuIndex)
{
    initCore(gpuIndex);
    createCommandObjects();
    createSwapChain();
    createDescriptorHeaps();

    onResize();

    createShaders();
    createRootSignatureAndPSO();

    LOG("DX12 renderer initialized");
}

void DX12Renderer::initCore(unsigned short gpuIndex)
{
#ifdef DEBUG
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            LOG("DX12 debug layer enabled");
        }
    }
#endif

    HR(CreateDXGIFactory1(IID_PPV_ARGS(&mpDxgiFactory)));

    std::vector<ComPtr<IDXGIAdapter1>> adapters;
    IDXGIAdapter1* pAdapter;
    for (UINT i = 0; mpDxgiFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        adapters.emplace_back(pAdapter);
    }

    if (adapters.empty())
    {
        ERR("No GPU detected!");
    }

    if (gpuIndex >= adapters.size())
    {
        ERR(std::format("GPU index {} doesn't exist", gpuIndex));
    }

    IDXGIAdapter1* pSelectedAdapter = adapters[gpuIndex].Get();

    DXGI_ADAPTER_DESC1 adapterDesc;
    HR(pSelectedAdapter->GetDesc1(&adapterDesc));

    WLOG(std::format(L"Selected GPU: {}", adapterDesc.Description).data());

    HRESULT hr = D3D12CreateDevice(pSelectedAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mpDevice));
    if (FAILED(hr))
    {
        // Fallback to WARP adapter
        WARN("Hardware adapter failed, falling back to WARP");
        ComPtr<IDXGIAdapter> pWarpAdapter;
        HR(mpDxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
        HR(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mpDevice)));
    }
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

    HR(mpDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mpCommandAllocators[0].Get(), nullptr,
                                   IID_PPV_ARGS(&mpCommandList)));

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
    HR(mpDxgiFactory->CreateSwapChainForHwnd(mpCommandQueue.Get(), pWindow->getHwnd(), &swapChainDesc, nullptr,
                                             nullptr, &swapChain));

    HR(mpDxgiFactory->MakeWindowAssociation(pWindow->getHwnd(), DXGI_MWA_NO_ALT_ENTER));

    HR(swapChain.As(&mpSwapChain));
    mFrameIndex = mpSwapChain->GetCurrentBackBufferIndex();
}

void DX12Renderer::createDescriptorHeaps()
{
    // RTV heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = FrameCount,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    };
    HR(mpDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mpRtvHeap)));
    mRtvDescriptorSize = mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // DSV heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        .NumDescriptors = 1,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    };
    HR(mpDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mpDsvHeap)));

    // CBV/SRV/UAV heap (for future use, e.g. ImGui)
    D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc{
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = 256,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
    };
    HR(mpDevice->CreateDescriptorHeap(&cbvSrvUavHeapDesc, IID_PPV_ARGS(&mpCbvSrvUavHeap)));
}

void DX12Renderer::createRenderTargetViews()
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mpRtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < FrameCount; ++i)
    {
        HR(mpSwapChain->GetBuffer(i, IID_PPV_ARGS(&mpRenderTargets[i])));
        mpDevice->CreateRenderTargetView(mpRenderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += mRtvDescriptorSize;
    }
}

void DX12Renderer::createDepthStencilBuffer()
{
    const auto pWindow = mpWindow.lock().get();
    ASSERT(pWindow);

    D3D12_RESOURCE_DESC depthStencilDesc{
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = 0,
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

    D3D12_HEAP_PROPERTIES heapProps{
        .Type = D3D12_HEAP_TYPE_DEFAULT,
    };

    HR(mpDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthStencilDesc,
                                         D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
                                         IID_PPV_ARGS(&mpDepthStencilBuffer)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{
        .Format = DepthStencilFormat,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        .Flags = D3D12_DSV_FLAG_NONE,
        .Texture2D{.MipSlice = 0},
    };
    mpDevice->CreateDepthStencilView(mpDepthStencilBuffer.Get(), &dsvDesc,
                                     mpDsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void DX12Renderer::onResize()
{
    ASSERT(mpDevice);
    ASSERT(mpSwapChain);
    ASSERT(mpCommandAllocators[mFrameIndex]);

    waitForGpu();

    HR(mpCommandList->Reset(mpCommandAllocators[mFrameIndex].Get(), nullptr));

    // Release old resources
    for (UINT i = 0; i < FrameCount; ++i)
    {
        mpRenderTargets[i].Reset();
    }
    mpDepthStencilBuffer.Reset();

    const auto pWindow = mpWindow.lock().get();
    ASSERT(pWindow);

    HR(mpSwapChain->ResizeBuffers(FrameCount, pWindow->getWidth(), pWindow->getHeight(), BackBufferFormat, 0));
    mFrameIndex = mpSwapChain->GetCurrentBackBufferIndex();

    createRenderTargetViews();
    createDepthStencilBuffer();

    HR(mpCommandList->Close());

    ID3D12CommandList* cmdLists[] = {mpCommandList.Get()};
    mpCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    waitForGpu();

    mViewport = {
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = static_cast<float>(pWindow->getWidth()),
        .Height = static_cast<float>(pWindow->getHeight()),
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    };

    mScissorRect = {
        .left = 0,
        .top = 0,
        .right = static_cast<LONG>(pWindow->getWidth()),
        .bottom = static_cast<LONG>(pWindow->getHeight()),
    };
}

void DX12Renderer::createShaders()
{
    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef DEBUG
    shaderFlags |= D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

    auto compileShader = [shaderFlags](const fs::path& shaderPath, const char* target,
                                       ComPtr<ID3DBlob>& blob) {
        const fs::path fullPath = SHADERS_DIR / shaderPath;

        if (!fs::exists(fullPath))
        {
            ERR("Shader file doesn't exist: " + (fs::current_path() / fullPath).string());
        }

        ComPtr<ID3DBlob> errors;
        HRESULT hr = D3DCompileFromFile(fullPath.c_str(), nullptr, nullptr, "main", target, shaderFlags, 0,
                                        blob.GetAddressOf(), errors.GetAddressOf());
        if (FAILED(hr))
        {
            std::string msg = errors ? reinterpret_cast<char*>(errors->GetBufferPointer()) : "Unknown error";
            ERR("Shader compilation failed: " + msg);
        }
    };

    compileShader("default_renderable/default_renderable_vs.hlsl", "vs_5_0", mShaders.defaultVS);
    compileShader("default_renderable/default_renderable_ps.hlsl", "ps_5_0", mShaders.defaultPS);

    LOG("DX12 shader compilation SUCCESS");
}

void DX12Renderer::createRootSignatureAndPSO()
{
    // Root signature with a single root constant buffer
    D3D12_ROOT_PARAMETER rootParam{
        .ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
        .Descriptor{.ShaderRegister = 0, .RegisterSpace = 0},
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
    };

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc{
        .NumParameters = 1,
        .pParameters = &rootParam,
        .NumStaticSamplers = 0,
        .pStaticSamplers = nullptr,
        .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
    };

    ComPtr<ID3DBlob> serializedRootSig;
    ComPtr<ID3DBlob> errorBlob;
    HR(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob));
    HR(mpDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(),
                                     IID_PPV_ARGS(&mpRootSignature)));

    // Default PSO
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.InputLayout = {inputLayout, _countof(inputLayout)};
    psoDesc.pRootSignature = mpRootSignature.Get();
    psoDesc.VS = {mShaders.defaultVS->GetBufferPointer(), mShaders.defaultVS->GetBufferSize()};
    psoDesc.PS = {mShaders.defaultPS->GetBufferPointer(), mShaders.defaultPS->GetBufferSize()};

    psoDesc.RasterizerState = {
        .FillMode = D3D12_FILL_MODE_SOLID,
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

    psoDesc.BlendState = {
        .AlphaToCoverageEnable = FALSE,
        .IndependentBlendEnable = FALSE,
        .RenderTarget = {{
            .BlendEnable = TRUE,
            .SrcBlend = D3D12_BLEND_SRC_ALPHA,
            .DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
            .BlendOp = D3D12_BLEND_OP_ADD,
            .SrcBlendAlpha = D3D12_BLEND_ONE,
            .DestBlendAlpha = D3D12_BLEND_ZERO,
            .BlendOpAlpha = D3D12_BLEND_OP_ADD,
            .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
        }},
    };

    psoDesc.DepthStencilState = {
        .DepthEnable = TRUE,
        .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc = D3D12_COMPARISON_FUNC_LESS,
        .StencilEnable = FALSE,
    };

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = BackBufferFormat;
    psoDesc.DSVFormat = DepthStencilFormat;
    psoDesc.SampleDesc = {.Count = 1, .Quality = 0};

    HR(mpDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mpDefaultPSO)));
}

void DX12Renderer::beginFrame(float clearR, float clearG, float clearB, float clearA)
{
    HR(mpCommandAllocators[mFrameIndex]->Reset());
    HR(mpCommandList->Reset(mpCommandAllocators[mFrameIndex].Get(), nullptr));

    // Transition render target to RENDER_TARGET state
    D3D12_RESOURCE_BARRIER barrier{
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition{
            .pResource = mpRenderTargets[mFrameIndex].Get(),
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
            .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
        },
    };
    mpCommandList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mpRtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += mFrameIndex * mRtvDescriptorSize;

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mpDsvHeap->GetCPUDescriptorHandleForHeapStart();

    mpCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    const float clearColor[] = {clearR, clearG, clearB, clearA};
    mpCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    mpCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0,
                                         nullptr);

    mpCommandList->RSSetViewports(1, &mViewport);
    mpCommandList->RSSetScissorRects(1, &mScissorRect);
}

void DX12Renderer::endFrame(bool vsync)
{
    // Transition render target to PRESENT state
    D3D12_RESOURCE_BARRIER barrier{
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition{
            .pResource = mpRenderTargets[mFrameIndex].Get(),
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
            .StateAfter = D3D12_RESOURCE_STATE_PRESENT,
        },
    };
    mpCommandList->ResourceBarrier(1, &barrier);

    HR(mpCommandList->Close());

    ID3D12CommandList* cmdLists[] = {mpCommandList.Get()};
    mpCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    HR(mpSwapChain->Present(vsync ? 1 : 0, 0));

    moveToNextFrame();
}

void DX12Renderer::waitForGpu()
{
    if (mpCommandQueue == nullptr || mpFence == nullptr || mFenceEvent == nullptr)
    {
        return;
    }

    const UINT64 fenceValue = mFenceValues[mFrameIndex];
    HR(mpCommandQueue->Signal(mpFence.Get(), fenceValue));

    HR(mpFence->SetEventOnCompletion(fenceValue, mFenceEvent));
    WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);

    mFenceValues[mFrameIndex]++;
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

        if (geometry.empty())
        {
            ASSERT(false);
            continue;
        }

        mVertexBuffers[i].Reset();
        mIndexBuffers[i].Reset();

        const UINT vbByteSize = static_cast<UINT>(sizeof(XMFLOAT3) * geometry.size());

        // Create upload buffer for vertices
        D3D12_HEAP_PROPERTIES uploadHeapProps{.Type = D3D12_HEAP_TYPE_UPLOAD};
        D3D12_RESOURCE_DESC bufferDesc{
            .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment = 0,
            .Width = vbByteSize,
            .Height = 1,
            .DepthOrArraySize = 1,
            .MipLevels = 1,
            .Format = DXGI_FORMAT_UNKNOWN,
            .SampleDesc{.Count = 1, .Quality = 0},
            .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags = D3D12_RESOURCE_FLAG_NONE,
        };

        HR(mpDevice->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                             D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                             IID_PPV_ARGS(&mVertexBuffers[i])));

        // Copy vertex data
        void* mappedData = nullptr;
        D3D12_RANGE readRange{0, 0};
        HR(mVertexBuffers[i]->Map(0, &readRange, &mappedData));
        memcpy(mappedData, geometry.data(), vbByteSize);
        mVertexBuffers[i]->Unmap(0, nullptr);

        mVertexBufferViews[i] = {
            .BufferLocation = mVertexBuffers[i]->GetGPUVirtualAddress(),
            .SizeInBytes = vbByteSize,
            .StrideInBytes = sizeof(XMFLOAT3),
        };

        if (!topology.empty())
        {
            const UINT ibByteSize = static_cast<UINT>(sizeof(unsigned) * topology.size());

            bufferDesc.Width = ibByteSize;

            HR(mpDevice->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                 IID_PPV_ARGS(&mIndexBuffers[i])));

            HR(mIndexBuffers[i]->Map(0, &readRange, &mappedData));
            memcpy(mappedData, topology.data(), ibByteSize);
            mIndexBuffers[i]->Unmap(0, nullptr);

            mIndexBufferViews[i] = {
                .BufferLocation = mIndexBuffers[i]->GetGPUVirtualAddress(),
                .SizeInBytes = ibByteSize,
                .Format = DXGI_FORMAT_R32_UINT,
            };
        }

        pRenderable->mShouldRebuildBuffers = false;
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
        mIndexBuffers.emplace_back();
        mVertexBufferViews.emplace_back();
        mIndexBufferViews.emplace_back();
    }
    else
    {
        static constexpr size_t reasonablyEmptyAmountPredecessors = 500;
        ASSERT(!((renderableId - mRenderablePtrs.size()) > reasonablyEmptyAmountPredecessors));

        mRenderablePtrs.resize(renderableId + 1);
        mRenderablePtrs[renderableId] = std::move(pRenderable);
        mVertexBuffers.resize(renderableId + 1, nullptr);
        mIndexBuffers.resize(renderableId + 1, nullptr);
        mVertexBufferViews.resize(renderableId + 1);
        mIndexBufferViews.resize(renderableId + 1);
    }

    ASSERT(mRenderablePtrs[renderableId] != nullptr);
    mRenderablePtrs[renderableId]->regenerateData();
}

void DX12Renderer::addRenderable(std::vector<std::unique_ptr<IRenderable>>&& renderables)
{
    for (auto& renderable : renderables)
    {
        addRenderable(std::move(renderable));
    }
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
    (mVertexBuffers.begin() + id)->Reset();
    (mIndexBuffers.begin() + id)->Reset();
    mVertexBufferViews[id] = {};
    mIndexBufferViews[id] = {};

    IIdentifiable::mFreeIds.push(id);
}

void DX12Renderer::removeRenderable(std::vector<Id> ids)
{
    for (Id id : ids)
    {
        removeRenderable(id);
    }
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
