module;

#include "utils.h"
#include "imgui_impl_dx12.h"

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <array>
#include <format>
#include <vector>

export module dx12renderer;

import std;
import dxrenderer;
import platform;

export class DX12Renderer final : public DXRenderer
{
    static constexpr UINT frameCount = 2u;

public:
    DX12Renderer(std::weak_ptr<IWindow> pWindow) : DXRenderer{pWindow}
    {
    }

    ~DX12Renderer()
    {
        clearRenderables();
        ImGui_ImplDX12_Shutdown();

        if (mpCommandQueue && mpFence)
        {
            flushCommandQueue();
        }
    }

    void init(unsigned short gpuIndex) override;
    void onResize() override;

    void beginFrame(const XMFLOAT4& clearColor) override;
    void endFrame(bool isVsync) override;

    void buildGeometryBuffers() override
    {
        ASSERT(false);
    }

    ComPtr<ID3D11ShaderResourceView> createShaderResourceView(const fs::path& /*texPath*/) const override
    {
        ASSERT(false);
        return {};
    }

    void createShaders() override
    {
        ASSERT(false);
    }

    void setSolidRaster(bool /*isCull*/ = false) override
    {
        ASSERT(false);
    }

    void setWireframeRaster(bool /*isCull*/ = false) override
    {
        ASSERT(false);
    }

    ID3D11Device* getDevice() const override
    {
        return nullptr;
    }

    ID3D11DeviceContext* getContext() const override
    {
        return nullptr;
    }

    ID3D11RenderTargetView* getRenderTargetView() const override
    {
        return nullptr;
    }

    ID3D11RenderTargetView* const* getAddressOfRenderTargetView() const override
    {
        return nullptr;
    }

    IDXGISwapChain* getSwapchain() const override
    {
        return mpSwapChain.Get();
    }

    ID3D12GraphicsCommandList* getCommandList() const
    {
        return mpCommandList.Get();
    }

    ID3D12Device* getD3DDevice() const
    {
        return mpDevice.Get();
    }

    ID3D12DescriptorHeap* getImguiSrvHeap() const
    {
        return mpImguiSrvHeap.Get();
    }

    ID3D11DepthStencilView* getDepthStencilView() const override
    {
        return nullptr;
    }

    ID3D11InputLayout* getDefaultInputLayout() const override
    {
        return nullptr;
    }

    ID3D11InputLayout* getBezierCurveInputLayout() const override
    {
        return nullptr;
    }

    ID3D11InputLayout* getBezierPatchInputLayout() const override
    {
        return nullptr;
    }

    const D3D11_VIEWPORT* getAddressOfScreenViewport() const override
    {
        return nullptr;
    }

    ID3D11Buffer* const* getVertexBufferAddress(size_t /*index*/) const override
    {
        ASSERT(false);
        return nullptr;
    }

    ID3D11Buffer* getIndexBuffer(size_t /*index*/) const override
    {
        ASSERT(false);
        return nullptr;
    }

    void addRenderable(std::unique_ptr<IRenderable>&& renderable) override;
    void addRenderable(std::vector<std::unique_ptr<IRenderable>>&& renderables) override
    {
        // TODO: implement batch insertion for DX12 path
        ASSERT(false);
    }

    void removeRenderable(Id id) override;
    void removeRenderable(std::vector<Id> ids) override
    {
        // TODO: implement batch removal for DX12 path
        ASSERT(false);
    }

    IRenderable* getRenderable(Id id) const override;
private:
    void initCore(unsigned short gpuIndex);
    void createSwapChain();
    void createCommandObjects();
    void createDescriptorHeaps();
    void createImguiDescriptorHeap();
    void flushCommandQueue();

    void buildRenderTargetViews();
    void buildDepthStencilView();

private:
    ComPtr<IDXGIFactory4> mpFactory;
    ComPtr<ID3D12Device> mpDevice;
    ComPtr<ID3D12Fence> mpFence;
    UINT64 mCurrentFence{};

    ComPtr<ID3D12CommandQueue> mpCommandQueue;
    ComPtr<ID3D12CommandAllocator> mpDirectCmdListAlloc;
    ComPtr<ID3D12GraphicsCommandList> mpCommandList;

    ComPtr<IDXGISwapChain3> mpSwapChain;

    ComPtr<ID3D12DescriptorHeap> mpRtvHeap;
    ComPtr<ID3D12DescriptorHeap> mpDsvHeap;
    ComPtr<ID3D12DescriptorHeap> mpImguiSrvHeap;

    std::array<ComPtr<ID3D12Resource>, frameCount> mSwapChainBuffer;
    ComPtr<ID3D12Resource> mpDepthStencilBuffer;

    UINT mCurrentBackBuffer{};
    UINT mRtvDescriptorSize{};
    UINT mDsvDescriptorSize{};

    D3D12_VIEWPORT mScreenViewport{};
    D3D12_RECT mScissorRect{};

    std::vector<ComPtr<ID3D12Resource>> mVertexBuffers;
    std::vector<ComPtr<ID3D12Resource>> mIndexBuffers;
};

module :private;

using namespace std;

namespace
{
    [[nodiscard]] vector<ComPtr<IDXGIAdapter1>> enumerateAdapters(ComPtr<IDXGIFactory4> pFactory)
    {
        vector<ComPtr<IDXGIAdapter1>> adapters;
        ComPtr<IDXGIAdapter1> adapter;
        for (UINT i = 0; pFactory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            adapters.push_back(adapter);
        }

        return adapters;
    }
}

void DX12Renderer::init(unsigned short gpuIndex)
{
    initCore(gpuIndex);
    createDescriptorHeaps();
    createImguiDescriptorHeap();

    ImGui_ImplDX12_Init(mpDevice.Get(), frameCount, DXGI_FORMAT_R8G8B8A8_UNORM,
                        mpImguiSrvHeap.Get(), mpImguiSrvHeap->GetCPUDescriptorHandleForHeapStart(),
                        mpImguiSrvHeap->GetGPUDescriptorHandleForHeapStart());
    onResize();
}

void DX12Renderer::initCore(unsigned short gpuIndex)
{
#ifdef DEBUG
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }

    UINT dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#else
    UINT dxgiFactoryFlags = 0u;
#endif

    HR(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mpFactory)));

    auto adapters = enumerateAdapters(mpFactory);

    if (adapters.empty())
    {
        ERR("No GPU detected!");
    }

    if (gpuIndex >= adapters.size())
    {
        ERR(std::format("Following {} gpu index doesn't exist", gpuIndex));
    }

    ComPtr<IDXGIAdapter1> pSelectedAdapter = adapters[gpuIndex];

    DXGI_ADAPTER_DESC1 adapterDesc;
    HR(pSelectedAdapter->GetDesc1(&adapterDesc));
    const std::wstring adapterName = std::format(L"Selected GPU: {}", adapterDesc.Description);
    WLOG(adapterName);

    HR(D3D12CreateDevice(pSelectedAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mpDevice)));

    HR(mpDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mpFence)));
    mCurrentFence = 0;

    mRtvDescriptorSize = mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDsvDescriptorSize = mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    createCommandObjects();
    createSwapChain();
}

void DX12Renderer::createCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    HR(mpDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mpCommandQueue)));

    HR(mpDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mpDirectCmdListAlloc)));
    HR(mpDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mpDirectCmdListAlloc.Get(), nullptr,
                                   IID_PPV_ARGS(&mpCommandList)));

    HR(mpCommandList->Close());
}

void DX12Renderer::createSwapChain()
{
    ASSERT(mpFactory);
    ASSERT(mpCommandQueue);

    auto pWindow = mpWindow.lock();
    ASSERT(pWindow);

    auto pWin32Window = dynamic_cast<Win32Window*>(pWindow.get());
    ASSERT(pWin32Window != nullptr);

    mpSwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC1 sd{};
    sd.BufferCount = frameCount;
    sd.Width = static_cast<UINT>(pWindow->getWidth());
    sd.Height = static_cast<UINT>(pWindow->getHeight());
    sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    HR(mpFactory->CreateSwapChainForHwnd(mpCommandQueue.Get(), pWin32Window->getHwnd(), &sd, nullptr, nullptr,
                                         &swapChain));

    HR(mpFactory->MakeWindowAssociation(pWin32Window->getHwnd(), DXGI_MWA_NO_ALT_ENTER));

    HR(swapChain.As(&mpSwapChain));
    mCurrentBackBuffer = 0;
}

void DX12Renderer::createDescriptorHeaps()
{
    ASSERT(mpDevice);

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = frameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HR(mpDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mpRtvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HR(mpDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mpDsvHeap)));
}

void DX12Renderer::createImguiDescriptorHeap()
{
    ASSERT(mpDevice);

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
    srvHeapDesc.NumDescriptors = frameCount;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    HR(mpDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mpImguiSrvHeap)));
}

void DX12Renderer::flushCommandQueue()
{
    ASSERT(mpCommandQueue);
    ASSERT(mpFence);

    ++mCurrentFence;
    HR(mpCommandQueue->Signal(mpFence.Get(), mCurrentFence));

    if (mpFence->GetCompletedValue() < mCurrentFence)
    {
        HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);
        ASSERT(eventHandle != nullptr);

        HR(mpFence->SetEventOnCompletion(mCurrentFence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void DX12Renderer::buildRenderTargetViews()
{
    ASSERT(mpDevice);
    ASSERT(mpSwapChain);
    ASSERT(mpRtvHeap);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mpRtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < frameCount; ++i)
    {
        HR(mpSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
        mpDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += mRtvDescriptorSize;
    }
}

void DX12Renderer::buildDepthStencilView()
{
    ASSERT(mpDevice);
    ASSERT(mpDsvHeap);

    auto pWindow = mpWindow.lock();
    ASSERT(pWindow);

    D3D12_RESOURCE_DESC depthStencilDesc{};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Width = static_cast<UINT>(pWindow->getWidth());
    depthStencilDesc.Height = static_cast<UINT>(pWindow->getHeight());
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE optClear{};
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    HR(mpDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthStencilDesc,
                                         D3D12_RESOURCE_STATE_COMMON, &optClear,
                                         IID_PPV_ARGS(&mpDepthStencilBuffer)));

    mpDevice->CreateDepthStencilView(mpDepthStencilBuffer.Get(), nullptr,
                                     mpDsvHeap->GetCPUDescriptorHandleForHeapStart());

    HR(mpDirectCmdListAlloc->Reset());
    HR(mpCommandList->Reset(mpDirectCmdListAlloc.Get(), nullptr));

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = mpDepthStencilBuffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    mpCommandList->ResourceBarrier(1, &barrier);
    HR(mpCommandList->Close());

    ID3D12CommandList* cmdsLists[] = {mpCommandList.Get()};
    mpCommandQueue->ExecuteCommandLists(1, cmdsLists);

    flushCommandQueue();
}

void DX12Renderer::onResize()
{
    ASSERT(mpDevice);
    ASSERT(mpSwapChain);

    flushCommandQueue();

    for (auto& buffer : mSwapChainBuffer)
    {
        buffer.Reset();
    }
    mpDepthStencilBuffer.Reset();

    auto pWindow = mpWindow.lock();
    ASSERT(pWindow);

    HR(mpSwapChain->ResizeBuffers(frameCount, static_cast<UINT>(pWindow->getWidth()),
                                  static_cast<UINT>(pWindow->getHeight()), DXGI_FORMAT_R8G8B8A8_UNORM, 0));

    mCurrentBackBuffer = 0;

    buildRenderTargetViews();
    buildDepthStencilView();

    mScreenViewport.TopLeftX = 0.0f;
    mScreenViewport.TopLeftY = 0.0f;
    mScreenViewport.Width = static_cast<float>(pWindow->getWidth());
    mScreenViewport.Height = static_cast<float>(pWindow->getHeight());
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;

    mScissorRect = {
        0,
        0,
        static_cast<LONG>(pWindow->getWidth()),
        static_cast<LONG>(pWindow->getHeight())};
}

void DX12Renderer::beginFrame(const XMFLOAT4& clearColor)
{
    ASSERT(mpCommandQueue);
    ASSERT(mpCommandList);
    ASSERT(mpDirectCmdListAlloc);
    ASSERT(mSwapChainBuffer[mCurrentBackBuffer]);

    HR(mpDirectCmdListAlloc->Reset());
    HR(mpCommandList->Reset(mpDirectCmdListAlloc.Get(), nullptr));

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = mSwapChainBuffer[mCurrentBackBuffer].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    mpCommandList->ResourceBarrier(1, &barrier);

    auto rtvHandle = mpRtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += static_cast<SIZE_T>(mCurrentBackBuffer) * mRtvDescriptorSize;

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mpDsvHeap->GetCPUDescriptorHandleForHeapStart();

    mpCommandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);
    mpCommandList->RSSetViewports(1, &mScreenViewport);
    mpCommandList->RSSetScissorRects(1, &mScissorRect);
    mpCommandList->ClearRenderTargetView(rtvHandle, reinterpret_cast<const float*>(&clearColor), 0, nullptr);
    mpCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
}

void DX12Renderer::endFrame(bool isVsync)
{
    ASSERT(mpCommandQueue);
    ASSERT(mpCommandList);

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = mSwapChainBuffer[mCurrentBackBuffer].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    mpCommandList->ResourceBarrier(1, &barrier);

    HR(mpCommandList->Close());

    ID3D12CommandList* cmdsLists[] = {mpCommandList.Get()};
    mpCommandQueue->ExecuteCommandLists(1, cmdsLists);

    HR(mpSwapChain->Present(isVsync ? 1u : 0u, 0));

    mCurrentBackBuffer = (mCurrentBackBuffer + 1u) % frameCount;

    flushCommandQueue();
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
    }
    else
    {
        static constexpr size_t reasonablyEmptyAmountPredecessors = 500;
        ASSERT(!((renderableId - mRenderablePtrs.size()) > reasonablyEmptyAmountPredecessors));

        mRenderablePtrs.resize(renderableId + 1);
        mRenderablePtrs[renderableId] = std::move(pRenderable);
        mVertexBuffers.resize(renderableId + 1, nullptr);
        mIndexBuffers.resize(renderableId + 1, nullptr);
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
    (mVertexBuffers.begin() + id)->Reset();
    (mIndexBuffers.begin() + id)->Reset();

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
