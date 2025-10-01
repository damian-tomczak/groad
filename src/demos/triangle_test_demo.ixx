module;

#include "utils.h"

#include <d3d11.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include <array>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>

export module triangle_test_demo;

export import core.demo;
import dxrenderer;
import dx11renderer;
import dx12renderer;

using Microsoft::WRL::ComPtr;

namespace
{
    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    constexpr std::array<Vertex, 3> triangleVertices{
        Vertex{DirectX::XMFLOAT3{0.0f, 0.5f, 0.0f}, DirectX::XMFLOAT4{1.0f, 0.0f, 0.0f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, -0.5f, 0.0f}, DirectX::XMFLOAT4{0.0f, 1.0f, 0.0f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{-0.5f, -0.5f, 0.0f}, DirectX::XMFLOAT4{0.0f, 0.0f, 1.0f, 1.0f}},
    };

    [[nodiscard]] ComPtr<ID3DBlob> compileShader(std::string_view source, std::string_view entryPoint,
                                                 std::string_view target)
    {
        ComPtr<ID3DBlob> shaderBlob;
        ComPtr<ID3DBlob> errorBlob;

        const HRESULT hr = D3DCompile(source.data(), source.size(), nullptr, nullptr, nullptr,
                                      entryPoint.data(), target.data(), 0u, 0u,
                                      shaderBlob.GetAddressOf(), errorBlob.GetAddressOf());

        if (FAILED(hr))
        {
            std::string errorMessage = "Failed to compile triangle test shader";
            if (errorBlob != nullptr)
            {
                errorMessage += std::string{": "} + static_cast<const char*>(errorBlob->GetBufferPointer());
            }

            ERR(errorMessage.c_str());
        }

        return shaderBlob;
    }

    class ITriangleDemoRenderer
    {
    public:
        virtual ~ITriangleDemoRenderer() = default;

        virtual void init() = 0;
        virtual void draw() = 0;
    };

    class TriangleTestRendererDX11 final : public ITriangleDemoRenderer
    {
    public:
        explicit TriangleTestRendererDX11(DX11Renderer& renderer) : mRenderer{renderer} {}

        void init() override;
        void draw() override;

    private:
        void createPipelineObjects();
        void createVertexBuffer();

    private:
        DX11Renderer& mRenderer;
        ComPtr<ID3D11VertexShader> mpVertexShader;
        ComPtr<ID3D11PixelShader> mpPixelShader;
        ComPtr<ID3D11InputLayout> mpInputLayout;
        ComPtr<ID3D11Buffer> mpVertexBuffer;
    };

    class TriangleTestRendererDX12 final : public ITriangleDemoRenderer
    {
    public:
        explicit TriangleTestRendererDX12(DX12Renderer& renderer) : mRenderer{renderer} {}

        void init() override;
        void draw() override;

    private:
        void createPipelineObjects();
        void createVertexBuffer();

    private:
        DX12Renderer& mRenderer;
        ComPtr<ID3D12RootSignature> mpRootSignature;
        ComPtr<ID3D12PipelineState> mpPipelineState;
        ComPtr<ID3D12Resource> mpVertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView{};
    };
}

export class TriangleTestDemo final : public IDemo
{
public:
    TriangleTestDemo(Context& ctx, DXRenderer* pRenderer, std::shared_ptr<IWindow> pWindow);

    void init() override;
    void draw() override;
    void processInput(IWindow::Message /*msg*/, float /*dt*/) override {}
    void update(float dt) override { IDemo::update(dt); }

private:
    std::unique_ptr<ITriangleDemoRenderer> mpTriangleRenderer;
};

module :private;

void TriangleTestRendererDX11::init()
{
    createPipelineObjects();
    createVertexBuffer();
}

void TriangleTestRendererDX11::draw()
{
    auto* const pContext = mRenderer.getContext();
    ASSERT(pContext != nullptr);

    UINT stride = sizeof(Vertex);
    UINT offset = 0u;
    ID3D11Buffer* const vertexBuffers[] = {mpVertexBuffer.Get()};

    pContext->IASetInputLayout(mpInputLayout.Get());
    pContext->IASetVertexBuffers(0u, 1u, vertexBuffers, &stride, &offset);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pContext->VSSetShader(mpVertexShader.Get(), nullptr, 0u);
    pContext->HSSetShader(nullptr, nullptr, 0u);
    pContext->DSSetShader(nullptr, nullptr, 0u);
    pContext->GSSetShader(nullptr, nullptr, 0u);
    pContext->PSSetShader(mpPixelShader.Get(), nullptr, 0u);

    pContext->Draw(static_cast<UINT>(triangleVertices.size()), 0u);
}

void TriangleTestRendererDX11::createPipelineObjects()
{
    ID3D11Device* const pDevice = mRenderer.getDevice();
    ASSERT(pDevice != nullptr);

    static constexpr std::string_view vertexShaderSrc =
        "struct VSInput { float3 position : POSITION; float4 color : COLOR; };\n"
        "struct PSInput { float4 position : SV_POSITION; float4 color : COLOR; };\n"
        "PSInput main(VSInput input) { PSInput output;\n"
        "    output.position = float4(input.position, 1.0f);\n"
        "    output.color = input.color;\n"
        "    return output;\n"
        "}\n";

    static constexpr std::string_view pixelShaderSrc =
        "struct PSInput { float4 position : SV_POSITION; float4 color : COLOR; };\n"
        "float4 main(PSInput input) : SV_TARGET { return input.color; }\n";

    ComPtr<ID3DBlob> vertexShader = compileShader(vertexShaderSrc, "main", "vs_5_0");
    ComPtr<ID3DBlob> pixelShader = compileShader(pixelShaderSrc, "main", "ps_5_0");

    HR(pDevice->CreateVertexShader(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), nullptr,
                                    mpVertexShader.GetAddressOf()));
    HR(pDevice->CreatePixelShader(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize(), nullptr,
                                   mpPixelShader.GetAddressOf()));

    const D3D11_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D11_INPUT_PER_VERTEX_DATA, 0u},
        {"COLOR", 0u, DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, 12u, D3D11_INPUT_PER_VERTEX_DATA, 0u},
    };

    HR(pDevice->CreateInputLayout(inputLayout, static_cast<UINT>(std::size(inputLayout)),
                                  vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(),
                                  mpInputLayout.GetAddressOf()));
}

void TriangleTestRendererDX11::createVertexBuffer()
{
    ID3D11Device* const pDevice = mRenderer.getDevice();
    ASSERT(pDevice != nullptr);

    const UINT vertexBufferSize = static_cast<UINT>(triangleVertices.size() * sizeof(Vertex));

    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = vertexBufferSize;
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = triangleVertices.data();

    HR(pDevice->CreateBuffer(&bufferDesc, &initData, mpVertexBuffer.GetAddressOf()));
}

void TriangleTestRendererDX12::init()
{
    createPipelineObjects();
    createVertexBuffer();
}

void TriangleTestRendererDX12::draw()
{
    auto* const pCommandList = mRenderer.getCommandList();
    ASSERT(pCommandList != nullptr);

    pCommandList->SetGraphicsRootSignature(mpRootSignature.Get());
    pCommandList->SetPipelineState(mpPipelineState.Get());

    const D3D12_VERTEX_BUFFER_VIEW vertexBufferView = mVertexBufferView;
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    pCommandList->DrawInstanced(static_cast<UINT>(triangleVertices.size()), 1u, 0u, 0u);
}

void TriangleTestRendererDX12::createPipelineObjects()
{
    ID3D12Device* const pDevice = mRenderer.getD3DDevice();
    ASSERT(pDevice != nullptr);

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> serializedSignature;
    ComPtr<ID3DBlob> errorBlob;
    HR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                   serializedSignature.GetAddressOf(), errorBlob.GetAddressOf()));

    if (errorBlob != nullptr)
    {
        WARN(static_cast<const char*>(errorBlob->GetBufferPointer()));
    }

    HR(pDevice->CreateRootSignature(0u, serializedSignature->GetBufferPointer(),
                                    serializedSignature->GetBufferSize(), IID_PPV_ARGS(&mpRootSignature)));

    static constexpr std::string_view vertexShaderSrc =
        "struct VSInput { float3 position : POSITION; float4 color : COLOR; };\n"
        "struct PSInput { float4 position : SV_POSITION; float4 color : COLOR; };\n"
        "PSInput main(VSInput input) { PSInput output;\n"
        "    output.position = float4(input.position, 1.0f);\n"
        "    output.color = input.color;\n"
        "    return output;\n"
        "}\n";

    static constexpr std::string_view pixelShaderSrc =
        "struct PSInput { float4 position : SV_POSITION; float4 color : COLOR; };\n"
        "float4 main(PSInput input) : SV_TARGET { return input.color; }\n";

    ComPtr<ID3DBlob> vertexShader = compileShader(vertexShaderSrc, "main", "vs_5_0");
    ComPtr<ID3DBlob> pixelShader = compileShader(pixelShaderSrc, "main", "ps_5_0");

    const D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u},
        {"COLOR", 0u, DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, 12u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u},
    };

    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthClipEnable = TRUE;

    D3D12_BLEND_DESC blendDesc{};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencilDesc.StencilEnable = FALSE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};
    pipelineDesc.InputLayout = {inputLayout, static_cast<UINT>(std::size(inputLayout))};
    pipelineDesc.pRootSignature = mpRootSignature.Get();
    pipelineDesc.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
    pipelineDesc.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
    pipelineDesc.RasterizerState = rasterizerDesc;
    pipelineDesc.BlendState = blendDesc;
    pipelineDesc.DepthStencilState = depthStencilDesc;
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.NumRenderTargets = 1u;
    pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pipelineDesc.SampleMask = UINT_MAX;
    pipelineDesc.SampleDesc.Count = 1u;
    pipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    HR(pDevice->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&mpPipelineState)));
}

void TriangleTestRendererDX12::createVertexBuffer()
{
    ID3D12Device* const pDevice = mRenderer.getD3DDevice();
    ASSERT(pDevice != nullptr);

    constexpr UINT vertexBufferSize = static_cast<UINT>(triangleVertices.size() * sizeof(Vertex));

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Alignment = 0u;
    resourceDesc.Width = vertexBufferSize;
    resourceDesc.Height = 1u;
    resourceDesc.DepthOrArraySize = 1u;
    resourceDesc.MipLevels = 1u;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1u;
    resourceDesc.SampleDesc.Quality = 0u;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HR(pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                        IID_PPV_ARGS(&mpVertexBuffer)));

    UINT8* pVertexDataBegin = nullptr;
    D3D12_RANGE readRange{0u, 0u};
    HR(mpVertexBuffer->Map(0u, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
    std::memcpy(pVertexDataBegin, triangleVertices.data(), vertexBufferSize);
    mpVertexBuffer->Unmap(0u, nullptr);

    mVertexBufferView.BufferLocation = mpVertexBuffer->GetGPUVirtualAddress();
    mVertexBufferView.StrideInBytes = sizeof(Vertex);
    mVertexBufferView.SizeInBytes = vertexBufferSize;
}

TriangleTestDemo::TriangleTestDemo(Context& ctx, DXRenderer* pRenderer, std::shared_ptr<IWindow> pWindow)
    : IDemo{ctx, "Triangle Test", pRenderer, pWindow, nullptr}
{
    ASSERT(pRenderer != nullptr);

    if (auto* const pDx12Renderer = dynamic_cast<DX12Renderer*>(pRenderer))
    {
        mpTriangleRenderer = std::make_unique<TriangleTestRendererDX12>(*pDx12Renderer);
    }
    else if (auto* const pDx11Renderer = dynamic_cast<DX11Renderer*>(pRenderer))
    {
        mpTriangleRenderer = std::make_unique<TriangleTestRendererDX11>(*pDx11Renderer);
    }
    else
    {
        ASSERT(false);
    }

    ASSERT(mpTriangleRenderer != nullptr);
}

void TriangleTestDemo::init()
{
    ASSERT(mpTriangleRenderer != nullptr);
    mpTriangleRenderer->init();
}

void TriangleTestDemo::draw()
{
    ASSERT(mpTriangleRenderer != nullptr);
    mpTriangleRenderer->draw();
}
