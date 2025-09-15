// DX12Framework-style skeleton (one file)
// - Win32 window + DX12 device/queue/swapchain
// - Modular pass system (RenderRoutine)
// - Shader hot-reload scaffold
// - Optional RenderDoc hooks
// Expand into headers/namespaces as you grow it.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>     // or DXC (dxcapi.h) if you prefer
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <cassert>
#include <cstdio>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

// If you use CD3DX12 helpers, include d3dx12.h from the Windows SDK samples:
#include "d3dx12.h"

// Optional: RenderDoc (press F12 to trigger capture)
#ifdef ENABLE_RENDERDOC
  #include "renderdoc_app.h"
  static RENDERDOC_API_1_6_0* g_rdoc = nullptr;
#endif

using Microsoft::WRL::ComPtr;

static const wchar_t* kAppName = L"DX12FrameworkSkeleton";
static uint32_t g_Width  = 1280;
static uint32_t g_Height = 720;
static const uint32_t kFrameCount = 3;

static HWND g_hWnd = nullptr;

// --- Minimal HR helper ---
static void HR(HRESULT hr) { if (FAILED(hr)) { assert(false); ::ExitProcess(UINT(hr)); } }

// --- Window proc ---
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l) {
  switch (msg) {
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_SIZE:    // TODO: handle resize (recreate swapchain RTVs, pass OnResize)
                     return 0;
    case WM_KEYDOWN:
      if (w == VK_ESCAPE) { DestroyWindow(hWnd); return 0; }
#ifdef ENABLE_RENDERDOC
      if (w == VK_F12 && g_rdoc) g_rdoc->TriggerCapture();
#endif
      return 0;
  }
  return DefWindowProc(hWnd, msg, w, l);
}

// ============================================================
//  Low-level DX12 device/swapchain + frame resources
// ============================================================
struct Frame {
  ComPtr<ID3D12CommandAllocator> cmdAlloc;
  UINT64 fenceValue = 0;
};

struct Dx12Context {
  // Core
  ComPtr<IDXGIFactory6>    factory;
  ComPtr<ID3D12Device>     device;
  ComPtr<ID3D12CommandQueue> queue;
  ComPtr<IDXGISwapChain4>  swapchain;
  ComPtr<ID3D12DescriptorHeap> rtvHeap;
  UINT rtvStride = 0;

  // Commanding
  ComPtr<ID3D12GraphicsCommandList> cmdList;
  ComPtr<ID3D12Fence> fence;
  HANDLE fenceEvent = nullptr;

  // Backbuffers
  ComPtr<ID3D12Resource> backbuffers[kFrameCount];
  D3D12_CPU_DESCRIPTOR_HANDLE rtvs[kFrameCount]{};
  UINT frameIndex = 0;

  // Frames
  Frame frames[kFrameCount];

  // Viewport/scissor
  D3D12_VIEWPORT vp{};
  D3D12_RECT     scissor{};

  void Init(HWND hwnd, uint32_t w, uint32_t h) {
#if _DEBUG
    // Enable debug layer
    ComPtr<ID3D12Debug> dbg;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg)))) dbg->EnableDebugLayer();
#endif
    UINT factoryFlags = 0;
#if _DEBUG
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    HR(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)));

    // Pick adapter
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i=0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
      DXGI_ADAPTER_DESC1 desc; adapter->GetDesc1(&desc);
      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
      if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
        break;
      adapter.Reset();
    }
    if (!adapter) HR(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
    HR(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));

    // Queue
    D3D12_COMMAND_QUEUE_DESC q{}; q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    HR(device->CreateCommandQueue(&q, IID_PPV_ARGS(&queue)));

    // Swapchain
    DXGI_SWAP_CHAIN_DESC1 scd{};
    scd.BufferCount = kFrameCount;
    scd.Width = w; scd.Height = h;
    scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> sc1;
    HR(factory->CreateSwapChainForHwnd(queue.Get(), hwnd, &scd, nullptr, nullptr, &sc1));
    HR(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
    HR(sc1.As(&swapchain));
    frameIndex = swapchain->GetCurrentBackBufferIndex();

    // RTV heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
    rtvDesc.NumDescriptors = kFrameCount;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    HR(device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap)));
    rtvStride = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Backbuffers + RTVs
    auto handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i=0; i<kFrameCount; ++i) {
      HR(swapchain->GetBuffer(i, IID_PPV_ARGS(&backbuffers[i])));
      device->CreateRenderTargetView(backbuffers[i].Get(), nullptr, handle);
      rtvs[i] = handle;
      handle.ptr += rtvStride;
    }

    // Command allocs & list
    for (UINT i=0; i<kFrameCount; ++i)
      HR(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frames[i].cmdAlloc)));
    HR(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frames[frameIndex].cmdAlloc.Get(), nullptr, IID_PPV_ARGS(&cmdList)));
    HR(cmdList->Close());

    // Fence
    HR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    frames[frameIndex].fenceValue = 1;

    // VP/scissor
    vp = { 0.f, 0.f, float(w), float(h), 0.f, 1.f };
    scissor = { 0, 0, LONG(w), LONG(h) };
  }

  void WaitForGPU() {
    const UINT64 fv = frames[frameIndex].fenceValue;
    HR(queue->Signal(fence.Get(), fv));
    if (fence->GetCompletedValue() < fv) {
      HR(fence->SetEventOnCompletion(fv, fenceEvent));
      WaitForSingleObject(fenceEvent, INFINITE);
    }
    frames[frameIndex].fenceValue++;
  }

  void BeginFrame() {
    HR(frames[frameIndex].cmdAlloc->Reset());
    HR(cmdList->Reset(frames[frameIndex].cmdAlloc.Get(), nullptr));

    D3D12_RESOURCE_BARRIER b = CD3DX12_RESOURCE_BARRIER::Transition(
      backbuffers[frameIndex].Get(),
      D3D12_RESOURCE_STATE_PRESENT,
      D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    cmdList->ResourceBarrier(1, &b);

    cmdList->RSSetViewports(1, &vp);
    cmdList->RSSetScissorRects(1, &scissor);
    auto rtv = rtvs[frameIndex];
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    const float clear[4] = {0.05f, 0.07f, 0.12f, 1.f};
    cmdList->ClearRenderTargetView(rtv, clear, 0, nullptr);
  }

  void EndFrame() {
    D3D12_RESOURCE_BARRIER b = CD3DX12_RESOURCE_BARRIER::Transition(
      backbuffers[frameIndex].Get(),
      D3D12_RESOURCE_STATE_RENDER_TARGET,
      D3D12_RESOURCE_STATE_PRESENT
    );
    cmdList->ResourceBarrier(1, &b);
    HR(cmdList->Close());
    ID3D12CommandList* lists[] = { cmdList.Get() };
    queue->ExecuteCommandLists(1, lists);
    HR(swapchain->Present(1, 0));
    frameIndex = swapchain->GetCurrentBackBufferIndex();
  }

  ~Dx12Context() {
    if (fenceEvent) { WaitForGPU(); CloseHandle(fenceEvent); }
  }
};

// ============================================================
//  Shader hot-reload scaffold (timestamp-based)
//  (DXC or D3DCompile; here we demo D3DCompile for brevity)
// ============================================================
struct ShaderFile {
  std::wstring path;
  std::chrono::file_clock::time_point lastWrite{};
  ComPtr<ID3DBlob> blob;
};

struct ShaderHotReload {
  std::vector<ShaderFile> tracked;

  void Track(const std::wstring& path) {
    ShaderFile f; f.path = path;
    if (std::filesystem::exists(path)) f.lastWrite = std::filesystem::last_write_time(path);
    tracked.push_back(std::move(f));
  }

  bool CompileIfChanged(ShaderFile& f, const char* entry, const char* target) {
    if (!std::filesystem::exists(f.path)) return false;
    auto now = std::filesystem::last_write_time(f.path);
    if (now == f.lastWrite && f.blob) return false;

    f.lastWrite = now;
    ComPtr<ID3DBlob> err;
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS
#if _DEBUG
               | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
#endif
               ;
    HRESULT hr = D3DCompileFromFile(f.path.c_str(), nullptr, nullptr, entry, target, flags, 0, &f.blob, &err);
    if (FAILED(hr)) {
      if (err) OutputDebugStringA((char*)err->GetBufferPointer());
      return false;
    }
    return true;
  }
};

// ============================================================
//  Pass system (“RenderRoutine”) — plug your FX here
// ============================================================
struct CommandContext {
  ID3D12GraphicsCommandList* cl = nullptr;
  // Add helpers: resource barriers, set PSO/rootsig, bind SRVs/UAVs, etc.
};

struct RenderRoutine {
  virtual ~RenderRoutine() {}
  virtual void OnCreate(Dx12Context& ctx) {}
  virtual void OnResize(Dx12Context& ctx, uint32_t w, uint32_t h) {}
  virtual void Record(Dx12Context& ctx, CommandContext& cmd) = 0;
};

struct ClearOverlayPass : RenderRoutine {
  // Example “post” pass placeholder
  void Record(Dx12Context& ctx, CommandContext& cmd) override {
    // Draw a full-screen triangle, composite, etc. (left as an exercise)
    // cmd.cl->SetPipelineState(...); cmd.cl->DrawInstanced(3, 1, 0, 0);
  }
};

// ============================================================
//  Renderer: holds routines, drives frame flow
// ============================================================
struct Renderer12 {
  Dx12Context* ctx = nullptr;
  std::vector<std::unique_ptr<RenderRoutine>> routines;

  void Init(Dx12Context& c) { ctx = &c; }
  template<typename T, typename...Args>
  T* AddRoutine(Args&&...args) {
    auto r = std::make_unique<T>(std::forward<Args>(args)...);
    r->OnCreate(*ctx);
    auto* raw = r.get();
    routines.push_back(std::move(r));
    return raw;
  }
  void OnResize(uint32_t w, uint32_t h) {
    for (auto& r : routines) r->OnResize(*ctx, w, h);
  }
  void Record() {
    CommandContext cc{ ctx->cmdList.Get() };
    for (auto& r : routines) r->Record(*ctx, cc);
  }
};

// ============================================================
//  App: window, loop, RenderDoc hook, hot-reload tick
// ============================================================
struct App {
  Dx12Context  dx;
  Renderer12   renderer;
  ShaderHotReload hot;

  void CreateWindowAndInitDX() {
    WNDCLASSEX wc{ sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"DX12FWWnd";
    RegisterClassEx(&wc);

    RECT rc{0,0,(LONG)g_Width,(LONG)g_Height};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    g_hWnd = CreateWindow(wc.lpszClassName, kAppName, WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT, rc.right-rc.left, rc.bottom-rc.top,
                          nullptr, nullptr, wc.hInstance, nullptr);
    ShowWindow(g_hWnd, SW_SHOW);

#ifdef ENABLE_RENDERDOC
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll")) {
      auto GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
      if (GetAPI) { int ok = GetAPI(eRENDERDOC_API_Version_1_6_0, (void**)&g_rdoc); (void)ok; }
    }
#endif

    dx.Init(g_hWnd, g_Width, g_Height);
    renderer.Init(dx);

    // Example pass
    renderer.AddRoutine<ClearOverlayPass>();

    // Example shader tracking
    // hot.Track(L"shaders/Fullscreen.hlsl");
  }

  void TickHotReload() {
    // For each tracked shader, recompile if changed and update PSOs as needed
    for (auto& f : hot.tracked) {
      // hot.CompileIfChanged(f, "PSMain", "ps_5_1"); // or "ps_6_0" with DXC
    }
  }

  int Run() {
    MSG msg{};
    bool running = true;
    while (running) {
      while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) { running = false; break; }
        TranslateMessage(&msg); DispatchMessage(&msg);
      }
      if (!running) break;

      TickHotReload();

#ifdef ENABLE_RENDERDOC
      // Optional first-frame capture when target control is connected:
      static bool first = true;
      if (first && g_rdoc && g_rdoc->IsTargetControlConnected()) {
        g_rdoc->StartFrameCapture(nullptr, nullptr);
        dx.BeginFrame();
        renderer.Record();
        dx.EndFrame();
        g_rdoc->EndFrameCapture(nullptr, nullptr);
        dx.WaitForGPU();
        first = false;
        continue;
      }
#endif

      dx.BeginFrame();
      renderer.Record();     // <- run your registered passes here
      dx.EndFrame();
      dx.WaitForGPU();
    }
    return 0;
  }
};

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
  App app;
  app.CreateWindowAndInitDX();
  return app.Run();
}
