module;

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <cstring>
#include <d3d12.h>
#include <wrl/client.h>

#include "imgui.h"

#include "cef/cef_runtime.h"
#include "utils.h"

export module cef_ui_demo;

import utils;
export import core.demo;

import dx11renderer;
import dx12renderer;
import platform;
import std;

using namespace Microsoft::WRL;

export class CEFUIDemo : public IDemo
{
public:
    CEFUIDemo(Context& ctx, IRenderer* pRenderer, std::shared_ptr<IWindow> pWindow);
    ~CEFUIDemo() override;

    void init() override;
    void update(float dt) override;
    void draw() override;
    void processInput(IWindow::Message msg, float dt) override;
    void renderUi() override;

private:
    struct Layout
    {
        int browserLeft{};
        int browserTop{};
        int browserWidth{};
        int browserHeight{};
        int inspectorLeft{};
        int inspectorTop{};
        int inspectorWidth{};
        int inspectorHeight{};
    };

    struct DX12OverlayResources;

    [[nodiscard]] bool isDx12() const;
    [[nodiscard]] Layout computeLayout() const;
    [[nodiscard]] fs::path resolveDemoPagePath() const;

    void ensureBrowser();
    void updateBrowserRect();
    void ensureOverlayResources(DX12Renderer& renderer, int width, int height);
    void recreateOverlayTexture(DX12Renderer& renderer, int width, int height);
    void ensureAcceleratedCopyResources(DX12Renderer& renderer);
    void copyAcceleratedBrowserFrame(HANDLE sharedTextureHandle, int width, int height);
    void uploadPendingBrowserFrame(DX12Renderer& renderer);
    void drawBrowserOverlay(DX12Renderer& renderer);
    void logEvent(std::string message);

    groad::cef::BrowserSurface mBrowser;
    std::unique_ptr<DX12OverlayResources> mDx12Overlay;

    fs::path mPagePath;
    std::deque<std::string> mEventLog;
    std::wstring mBrowserTitle = L"CEF Browser";

    int mLastFrameWidth = 0;
    int mLastFrameHeight = 0;
    size_t mLastFrameBytes = 0;

    bool mIsBrowserCreateAttempted = false;
    bool mIsBrowserReady = false;
    bool mIsUsingAcceleratedPaint = false;
    bool mHasLoggedAcceleratedPaint = false;
};

module :private;

namespace
{
constexpr UINT texturePixelSize = 4;

UINT alignTo(UINT value, UINT alignment)
{
    return (value + alignment - 1u) & ~(alignment - 1u);
}

class SceneGridSurface final : public ISurface
{
public:
    explicit SceneGridSurface(IRenderer* pRenderer) : ISurface{pRenderer}
    {
    }

    void init() override
    {
    }

    void update(float dt) override
    {
        (void)dt;
    }

    void draw() override
    {
        if (auto* pDX12Renderer = dynamic_cast<DX12Renderer*>(mpRenderer))
        {
            pDX12Renderer->setPSO(DX12Renderer::PSOType::GRID);
            pDX12Renderer->getCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            pDX12Renderer->getCommandList()->DrawInstanced(6, 1, 0, 0);
            return;
        }

        auto* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);
        pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().gridVS.first.Get(), nullptr, 0);
        pDX11Renderer->getContext()->HSSetShader(nullptr, nullptr, 0);
        pDX11Renderer->getContext()->DSSetShader(nullptr, nullptr, 0);
        pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
        pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().gridPS.first.Get(), nullptr, 0);
        pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pDX11Renderer->getContext()->Draw(6, 0);
    }
};
}

struct CEFUIDemo::DX12OverlayResources
{
    ~DX12OverlayResources()
    {
        if (copyFenceEvent != nullptr)
        {
            CloseHandle(copyFenceEvent);
            copyFenceEvent = nullptr;
        }
    }

    ComPtr<ID3D12Resource> texture;
    ComPtr<ID3D12Resource> uploadBuffer;
    ComPtr<ID3D12CommandAllocator> copyCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> copyCommandList;
    ComPtr<ID3D12Fence> copyFence;
    DX12Renderer::SrvDescriptorRange srvRange{};
    std::vector<ComPtr<ID3D12Resource>> retiredResources;

    UINT rowPitch = 0;
    UINT64 uploadSize = 0;
    UINT64 copyFenceValue = 0;
    int width = 0;
    int height = 0;
    HANDLE copyFenceEvent = nullptr;
    bool hasSrvRange = false;
};

CEFUIDemo::CEFUIDemo(Context& ctx, IRenderer* pRenderer, std::shared_ptr<IWindow> pWindow)
    : IDemo{ctx, "CEF UI Demo", pRenderer, pWindow, std::make_unique<SceneGridSurface>(pRenderer)}
{
    mpMenuBarCallback = [this]() {
        if (!ImGui::BeginMenu("CEF"))
        {
            return;
        }

        if (ImGui::MenuItem("Reload browser", "F5"))
        {
            mBrowser.reload();
            logEvent("Reload requested from the menu.");
        }

        ImGui::EndMenu();
    };
}

CEFUIDemo::~CEFUIDemo()
{
    mBrowser.setAcceleratedFrameCallback({});

    if (auto* pWindow = dynamic_cast<Win32Window*>(mpWindow.get()))
    {
        pWindow->clearNativeMessageHandler();
    }

    mBrowser.close();
}

bool CEFUIDemo::isDx12() const
{
    return dynamic_cast<DX12Renderer*>(mpRenderer) != nullptr;
}

CEFUIDemo::Layout CEFUIDemo::computeLayout() const
{
    const float margin = 24.0f;
    const float gap = 20.0f;
    const float windowWidth = static_cast<float>(mpWindow->getWidth());
    const float windowHeight = static_cast<float>(mpWindow->getHeight());
    const float topInset = ((mCtx.menuBarHeight > 0.0f) ? mCtx.menuBarHeight : 24.0f) + margin;
    const float inspectorWidth = std::clamp(windowWidth * 0.28f, 320.0f, 420.0f);
    const float browserWidth = std::max(320.0f, windowWidth - inspectorWidth - (margin * 2.0f) - gap);
    const float browserHeight = std::max(240.0f, windowHeight - topInset - margin);

    Layout layout{
        .browserLeft = static_cast<int>(margin),
        .browserTop = static_cast<int>(topInset),
        .browserWidth = static_cast<int>(browserWidth),
        .browserHeight = static_cast<int>(browserHeight),
    };
    layout.inspectorLeft = static_cast<int>(windowWidth - inspectorWidth - margin);
    layout.inspectorTop = layout.browserTop;
    layout.inspectorWidth = std::max(280, static_cast<int>(windowWidth) - layout.inspectorLeft - static_cast<int>(margin));
    layout.inspectorHeight = layout.browserHeight;
    return layout;
}

fs::path CEFUIDemo::resolveDemoPagePath() const
{
    std::array<wchar_t, MAX_PATH> executablePathBuffer{};
    const DWORD executablePathSize = GetModuleFileNameW(nullptr, executablePathBuffer.data(), static_cast<DWORD>(executablePathBuffer.size()));
    const fs::path executableDir = fs::path(std::wstring_view(executablePathBuffer.data(), executablePathSize)).parent_path();

    const std::array candidates{
        executableDir / "assets" / "cef" / "index.html",
        fs::current_path() / "assets" / "cef" / "index.html",
        fs::path("assets") / "cef" / "index.html",
    };

    for (const auto& candidate : candidates)
    {
        if (fs::exists(candidate))
        {
            return fs::absolute(candidate);
        }
    }

    return fs::absolute(candidates.front());
}

void CEFUIDemo::logEvent(std::string message)
{
    constexpr size_t maxEvents = 8;

    mEventLog.push_front(std::move(message));
    while (mEventLog.size() > maxEvents)
    {
        mEventLog.pop_back();
    }
}

void CEFUIDemo::init()
{
    IDemo::init();

    mPagePath = resolveDemoPagePath();

    if (isDx12())
    {
        mBrowser.setAcceleratedFrameCallback([this](HANDLE sharedTextureHandle, int width, int height) {
            copyAcceleratedBrowserFrame(sharedTextureHandle, width, height);
        });
    }

    if (auto* pWindow = dynamic_cast<Win32Window*>(mpWindow.get()))
    {
        pWindow->setNativeMessageHandler([this](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
            return mBrowser.handleMessage(hwnd, msg, wParam, lParam);
        });
    }

    updateBrowserRect();
    ensureBrowser();

    logEvent("Real CEF OSR scene initialized.");
}

void CEFUIDemo::ensureBrowser()
{
    if (mIsBrowserCreateAttempted || !isDx12() || !groad::cef::isInitialized())
    {
        return;
    }

    auto* pWindow = dynamic_cast<Win32Window*>(mpWindow.get());
    if (pWindow == nullptr)
    {
        logEvent("CEF demo requires the Win32 platform window.");
        mIsBrowserCreateAttempted = true;
        return;
    }

    mIsBrowserCreateAttempted = true;
    mIsBrowserReady = mBrowser.create(pWindow->getHwnd(), mPagePath);

    if (mIsBrowserReady)
    {
        logEvent(std::format("Windowless browser created from {}.", mPagePath.generic_string()));
    }
    else
    {
        logEvent("Failed to create the CEF browser surface.");
    }
}

void CEFUIDemo::updateBrowserRect()
{
    const Layout layout = computeLayout();
    mBrowser.setRect(layout.browserLeft, layout.browserTop, layout.browserWidth, layout.browserHeight);
}

void CEFUIDemo::update(float dt)
{
    IDemo::update(dt);
    (void)dt;

    updateBrowserRect();
    ensureBrowser();

    if (mIsBrowserReady)
    {
        mBrowser.requestFrame();
        mBrowserTitle = mBrowser.getTitle();
    }
}

void CEFUIDemo::ensureOverlayResources(DX12Renderer& renderer, int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        return;
    }

    if (mDx12Overlay == nullptr)
    {
        mDx12Overlay = std::make_unique<DX12OverlayResources>();
    }

    if (!mDx12Overlay->hasSrvRange)
    {
        mDx12Overlay->srvRange = renderer.allocateSrvDescriptorRange(2);
        mDx12Overlay->hasSrvRange = true;
    }

    if ((mDx12Overlay->texture == nullptr) || (mDx12Overlay->width != width) || (mDx12Overlay->height != height))
    {
        recreateOverlayTexture(renderer, width, height);
    }
}

void CEFUIDemo::recreateOverlayTexture(DX12Renderer& renderer, int width, int height)
{
    ASSERT(mDx12Overlay != nullptr);

    if (mDx12Overlay->texture != nullptr)
    {
        mDx12Overlay->retiredResources.push_back(mDx12Overlay->texture);
    }
    if (mDx12Overlay->uploadBuffer != nullptr)
    {
        mDx12Overlay->retiredResources.push_back(mDx12Overlay->uploadBuffer);
    }

    mDx12Overlay->texture.Reset();
    mDx12Overlay->uploadBuffer.Reset();

    mDx12Overlay->width = width;
    mDx12Overlay->height = height;
    mDx12Overlay->rowPitch = alignTo(static_cast<UINT>(width) * texturePixelSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    mDx12Overlay->uploadSize = static_cast<UINT64>(mDx12Overlay->rowPitch) * static_cast<UINT64>(height);

    D3D12_HEAP_PROPERTIES defaultHeap{
        .Type = D3D12_HEAP_TYPE_DEFAULT,
    };
    D3D12_RESOURCE_DESC textureDesc{
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Width = static_cast<UINT64>(width),
        .Height = static_cast<UINT>(height),
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
        .SampleDesc{.Count = 1, .Quality = 0},
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
    };

    HR(renderer.getDevice()->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_PPV_ARGS(&mDx12Overlay->texture)));

    D3D12_HEAP_PROPERTIES uploadHeap{
        .Type = D3D12_HEAP_TYPE_UPLOAD,
    };
    D3D12_RESOURCE_DESC uploadDesc{
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Width = mDx12Overlay->uploadSize,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .SampleDesc{.Count = 1, .Quality = 0},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
    };

    HR(renderer.getDevice()->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mDx12Overlay->uploadBuffer)));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
        .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Texture2D{
            .MostDetailedMip = 0,
            .MipLevels = 1,
            .PlaneSlice = 0,
            .ResourceMinLODClamp = 0.0f,
        },
    };

    renderer.getDevice()->CreateShaderResourceView(mDx12Overlay->texture.Get(), &srvDesc, mDx12Overlay->srvRange.cpuHandle);
    renderer.getDevice()->CreateShaderResourceView(
        mDx12Overlay->texture.Get(),
        &srvDesc,
        renderer.getCpuSrvHandle(mDx12Overlay->srvRange.startIndex + 1));

    logEvent(std::format("Allocated DX12 browser texture: {}x{}.", width, height));
}

void CEFUIDemo::ensureAcceleratedCopyResources(DX12Renderer& renderer)
{
    ASSERT(mDx12Overlay != nullptr);

    if (mDx12Overlay->copyCommandAllocator == nullptr)
    {
        HR(renderer.getDevice()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&mDx12Overlay->copyCommandAllocator)));
    }

    if (mDx12Overlay->copyCommandList == nullptr)
    {
        HR(renderer.getDevice()->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            mDx12Overlay->copyCommandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&mDx12Overlay->copyCommandList)));
        HR(mDx12Overlay->copyCommandList->Close());
    }

    if (mDx12Overlay->copyFence == nullptr)
    {
        HR(renderer.getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mDx12Overlay->copyFence)));
        mDx12Overlay->copyFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (mDx12Overlay->copyFenceEvent == nullptr)
        {
            ERR("Failed to create CEF accelerated copy fence event");
        }
    }
}

void CEFUIDemo::copyAcceleratedBrowserFrame(HANDLE sharedTextureHandle, int width, int height)
{
    auto* pDX12Renderer = dynamic_cast<DX12Renderer*>(mpRenderer);
    if ((pDX12Renderer == nullptr) || (sharedTextureHandle == nullptr) || (width <= 0) || (height <= 0))
    {
        return;
    }

    ensureOverlayResources(*pDX12Renderer, width, height);
    if ((mDx12Overlay == nullptr) || (mDx12Overlay->texture == nullptr))
    {
        return;
    }

    ensureAcceleratedCopyResources(*pDX12Renderer);
    if ((mDx12Overlay->copyCommandAllocator == nullptr)
        || (mDx12Overlay->copyCommandList == nullptr)
        || (mDx12Overlay->copyFence == nullptr)
        || (mDx12Overlay->copyFenceEvent == nullptr))
    {
        return;
    }

    ComPtr<ID3D12Resource> sharedTexture;
    const HRESULT openSharedHandleResult = pDX12Renderer->getDevice()->OpenSharedHandle(
        sharedTextureHandle,
        IID_PPV_ARGS(&sharedTexture));
    if (FAILED(openSharedHandleResult))
    {
        WARN(std::format("Failed to open accelerated CEF shared texture handle. {}({})",
            std::system_category().message(openSharedHandleResult),
            openSharedHandleResult));
        return;
    }

    HR(mDx12Overlay->copyCommandAllocator->Reset());
    HR(mDx12Overlay->copyCommandList->Reset(mDx12Overlay->copyCommandAllocator.Get(), nullptr));

    D3D12_RESOURCE_BARRIER toCopy{
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition{
            .pResource = mDx12Overlay->texture.Get(),
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            .StateAfter = D3D12_RESOURCE_STATE_COPY_DEST,
        },
    };
    mDx12Overlay->copyCommandList->ResourceBarrier(1, &toCopy);
    mDx12Overlay->copyCommandList->CopyResource(mDx12Overlay->texture.Get(), sharedTexture.Get());

    D3D12_RESOURCE_BARRIER toShaderRead{
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition{
            .pResource = mDx12Overlay->texture.Get(),
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
            .StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        },
    };
    mDx12Overlay->copyCommandList->ResourceBarrier(1, &toShaderRead);

    HR(mDx12Overlay->copyCommandList->Close());

    ID3D12CommandList* const commandLists[] = {mDx12Overlay->copyCommandList.Get()};
    pDX12Renderer->getCommandQueue()->ExecuteCommandLists(static_cast<UINT>(std::size(commandLists)), commandLists);

    mDx12Overlay->copyFenceValue += 1;
    HR(pDX12Renderer->getCommandQueue()->Signal(mDx12Overlay->copyFence.Get(), mDx12Overlay->copyFenceValue));
    HR(mDx12Overlay->copyFence->SetEventOnCompletion(mDx12Overlay->copyFenceValue, mDx12Overlay->copyFenceEvent));
    WaitForSingleObjectEx(mDx12Overlay->copyFenceEvent, INFINITE, FALSE);

    mIsUsingAcceleratedPaint = true;
    mLastFrameWidth = width;
    mLastFrameHeight = height;
    mLastFrameBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * texturePixelSize;

    if (!mHasLoggedAcceleratedPaint)
    {
        logEvent("Using CEF accelerated paint shared texture path.");
        mHasLoggedAcceleratedPaint = true;
    }
}

void CEFUIDemo::uploadPendingBrowserFrame(DX12Renderer& renderer)
{
    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    if (!mBrowser.consumeFrame(pixels, width, height))
    {
        return;
    }

    ensureOverlayResources(renderer, width, height);
    if ((mDx12Overlay == nullptr) || (mDx12Overlay->texture == nullptr) || pixels.empty())
    {
        return;
    }

    UINT8* pMappedData = nullptr;
    D3D12_RANGE readRange{0, 0};
    HR(mDx12Overlay->uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pMappedData)));

    const UINT srcRowPitch = static_cast<UINT>(width) * texturePixelSize;
    for (int row = 0; row < height; ++row)
    {
        std::memcpy(
            pMappedData + (static_cast<size_t>(row) * mDx12Overlay->rowPitch),
            pixels.data() + (static_cast<size_t>(row) * srcRowPitch),
            srcRowPitch);
    }

    mDx12Overlay->uploadBuffer->Unmap(0, nullptr);

    D3D12_RESOURCE_BARRIER toCopy{
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition{
            .pResource = mDx12Overlay->texture.Get(),
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            .StateAfter = D3D12_RESOURCE_STATE_COPY_DEST,
        },
    };
    renderer.getCommandList()->ResourceBarrier(1, &toCopy);

    D3D12_TEXTURE_COPY_LOCATION dstLocation{
        .pResource = mDx12Overlay->texture.Get(),
        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = 0,
    };
    D3D12_TEXTURE_COPY_LOCATION srcLocation{
        .pResource = mDx12Overlay->uploadBuffer.Get(),
        .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
        .PlacedFootprint{
            .Offset = 0,
            .Footprint{
                .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
                .Width = static_cast<UINT>(width),
                .Height = static_cast<UINT>(height),
                .Depth = 1,
                .RowPitch = mDx12Overlay->rowPitch,
            },
        },
    };
    renderer.getCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    D3D12_RESOURCE_BARRIER toShaderRead{
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition{
            .pResource = mDx12Overlay->texture.Get(),
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
            .StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        },
    };
    renderer.getCommandList()->ResourceBarrier(1, &toShaderRead);

    mLastFrameWidth = width;
    mLastFrameHeight = height;
    mLastFrameBytes = pixels.size();
}

void CEFUIDemo::drawBrowserOverlay(DX12Renderer& renderer)
{
    if (!mIsBrowserReady)
    {
        return;
    }

    if (!mIsUsingAcceleratedPaint)
    {
        uploadPendingBrowserFrame(renderer);
    }
    if ((mDx12Overlay == nullptr) || (mDx12Overlay->texture == nullptr))
    {
        return;
    }

    const Layout layout = computeLayout();
    const float windowWidth = static_cast<float>(mpWindow->getWidth());
    const float windowHeight = static_cast<float>(mpWindow->getHeight());
    const float scaleX = static_cast<float>(layout.browserWidth) / windowWidth;
    const float scaleY = static_cast<float>(layout.browserHeight) / windowHeight;
    const float centerX = ((static_cast<float>(layout.browserLeft) + (static_cast<float>(layout.browserWidth) * 0.5f)) / windowWidth) * 2.0f - 1.0f;
    const float centerY = 1.0f - ((static_cast<float>(layout.browserTop) + (static_cast<float>(layout.browserHeight) * 0.5f)) / windowHeight) * 2.0f;

    renderer.mGlobalCB.modelMtx = XMMatrixScaling(scaleX, scaleY, 1.0f) * XMMatrixTranslation(centerX, centerY, 0.0f);
    renderer.mGlobalCB.color = XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f};
    renderer.updateGlobalCB();

    renderer.setDefaultViewportAndScissor();
    renderer.setPSO(DX12Renderer::PSOType::CEF_OVERLAY);
    renderer.getCommandList()->SetGraphicsRootDescriptorTable(2, mDx12Overlay->srvRange.gpuHandle);
    renderer.getCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    renderer.getCommandList()->DrawInstanced(6, 1, 0, 0);
}

void CEFUIDemo::draw()
{
    IDemo::draw();

    if (auto* pDX12Renderer = dynamic_cast<DX12Renderer*>(mpRenderer))
    {
        drawBrowserOverlay(*pDX12Renderer);
    }
}

void CEFUIDemo::processInput(IWindow::Message msg, float dt)
{
    (void)dt;

    switch (msg)
    {
    case IWindow::Message::KEY_F5:
        if (mIsBrowserReady)
        {
            mBrowser.reload();
            logEvent("Reload requested from the keyboard.");
        }
        break;
    default:
        break;
    }
}

void CEFUIDemo::renderUi()
{
    const Layout layout = computeLayout();
    const ImGuiViewport* pViewport = ImGui::GetMainViewport();
    const ImVec2 viewportPos = pViewport->Pos;

    ImGui::SetNextWindowPos(ImVec2{
        viewportPos.x + static_cast<float>(layout.inspectorLeft),
        viewportPos.y + static_cast<float>(layout.inspectorTop)
    });
    ImGui::SetNextWindowSize(ImVec2{
        static_cast<float>(layout.inspectorWidth),
        static_cast<float>(layout.inspectorHeight)
    });

    const ImGuiWindowFlags inspectorFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("CEF Overlay Inspector", nullptr, inspectorFlags))
    {
        ImGui::TextWrapped("This scene uses a real CEF windowless browser, uploads the BGRA frame into a DX12 texture, and composites it over the live scene before ImGui.");

        if (!isDx12())
        {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4{1.0f, 0.63f, 0.2f, 1.0f}, "DX12 is required for the composited browser overlay.");
            ImGui::TextWrapped("Switch the application to D3D12 to see the Chromium surface rendered above the grid.");
        }
        else if (!groad::cef::isInitialized())
        {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4{1.0f, 0.45f, 0.45f, 1.0f}, "CEF runtime is not available.");
            ImGui::TextWrapped("Build with the CEF dependency enabled so the browser subprocess and runtime can start.");
        }
        else
        {
            if (ImGui::Button("Reload page"))
            {
                mBrowser.reload();
                logEvent("Reload requested from the inspector.");
            }

            ImGui::SeparatorText("Browser");
            ImGui::Text("Ready: %s", mIsBrowserReady ? "yes" : "waiting");
            ImGui::Text("Title: %ls", mBrowserTitle.c_str());
            ImGui::Text("Page: %s", mPagePath.generic_string().c_str());
            ImGui::Text("Last frame: %d x %d", mLastFrameWidth, mLastFrameHeight);
            ImGui::Text("Upload size: %.2f KiB", static_cast<float>(mLastFrameBytes) / 1024.0f);

            ImGui::SeparatorText("Layout");
            ImGui::Text("Browser rect: (%d, %d) %d x %d", layout.browserLeft, layout.browserTop, layout.browserWidth, layout.browserHeight);
            ImGui::Text("Inspector rect: (%d, %d) %d x %d", layout.inspectorLeft, layout.inspectorTop, layout.inspectorWidth, layout.inspectorHeight);

            ImGui::SeparatorText("Controls");
            ImGui::TextWrapped("Use the real browser surface on the left for pointer and keyboard input. Use right mouse + WASD outside that region to move the camera. F5 reloads the page.");

            ImGui::SeparatorText("Event log");
            ImGui::BeginChild("BrowserEventLog", ImVec2{0.0f, 0.0f}, true);
            for (const std::string& event : mEventLog)
            {
                ImGui::TextWrapped("%s", event.c_str());
                ImGui::Separator();
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();

    ImDrawList* pForeground = ImGui::GetForegroundDrawList();
    const ImVec2 browserMin{
        viewportPos.x + static_cast<float>(layout.browserLeft),
        viewportPos.y + static_cast<float>(layout.browserTop)
    };
    const ImVec2 browserMax{
        browserMin.x + static_cast<float>(layout.browserWidth),
        browserMin.y + static_cast<float>(layout.browserHeight)
    };
    pForeground->AddRect(browserMin, browserMax, IM_COL32(105, 210, 170, 220), 12.0f, 0, 2.0f);
    pForeground->AddText(ImVec2{browserMin.x + 10.0f, browserMin.y - 20.0f}, IM_COL32(190, 240, 225, 255), "CEF OSR Surface");

    const ImGuiIO& io = ImGui::GetIO();
    mCtx.isMenuHovered = io.WantCaptureMouse;
    mCtx.isUiClicked = io.WantCaptureMouse || io.WantCaptureKeyboard || ImGui::IsAnyItemActive();
}
