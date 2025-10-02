module;

#include "utils.h"

#include <DirectXMath.h>

#include <d3d12.h>
#include <wrl/client.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <span>

export module cube_demo;

export import core.demo;
import dx.render_backend;
import dxrenderer;
import dx12renderer;

namespace
{
    using Microsoft::WRL::ComPtr;

    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    constexpr std::array<Vertex, 36> cubeVertices{
        // Front face
        Vertex{DirectX::XMFLOAT3{-0.5f, -0.5f, 0.5f}, DirectX::XMFLOAT4{1.0f, 0.0f, 0.0f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{-0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT4{1.0f, 0.5f, 0.0f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT4{1.0f, 1.0f, 0.0f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{-0.5f, -0.5f, 0.5f}, DirectX::XMFLOAT4{1.0f, 0.0f, 0.0f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT4{1.0f, 1.0f, 0.0f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, -0.5f, 0.5f}, DirectX::XMFLOAT4{0.5f, 1.0f, 0.0f, 1.0f}},
        // Back face
        Vertex{DirectX::XMFLOAT3{-0.5f, -0.5f, -0.5f}, DirectX::XMFLOAT4{0.0f, 0.0f, 1.0f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, 0.5f, -0.5f}, DirectX::XMFLOAT4{0.0f, 1.0f, 1.0f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{-0.5f, 0.5f, -0.5f}, DirectX::XMFLOAT4{0.5f, 0.0f, 1.0f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{-0.5f, -0.5f, -0.5f}, DirectX::XMFLOAT4{0.0f, 0.0f, 1.0f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, -0.5f, -0.5f}, DirectX::XMFLOAT4{0.0f, 0.5f, 1.0f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, 0.5f, -0.5f}, DirectX::XMFLOAT4{0.0f, 1.0f, 1.0f, 1.0f}},
        // Left face
        Vertex{DirectX::XMFLOAT3{-0.5f, -0.5f, -0.5f}, DirectX::XMFLOAT4{0.5f, 0.5f, 0.5f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{-0.5f, 0.5f, -0.5f}, DirectX::XMFLOAT4{0.6f, 0.6f, 0.6f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{-0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT4{0.7f, 0.7f, 0.7f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{-0.5f, -0.5f, -0.5f}, DirectX::XMFLOAT4{0.5f, 0.5f, 0.5f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{-0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT4{0.7f, 0.7f, 0.7f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{-0.5f, -0.5f, 0.5f}, DirectX::XMFLOAT4{0.8f, 0.8f, 0.8f, 1.0f}},
        // Right face
        Vertex{DirectX::XMFLOAT3{0.5f, -0.5f, -0.5f}, DirectX::XMFLOAT4{0.4f, 0.2f, 0.8f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT4{0.6f, 0.3f, 0.9f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, 0.5f, -0.5f}, DirectX::XMFLOAT4{0.5f, 0.3f, 0.8f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, -0.5f, -0.5f}, DirectX::XMFLOAT4{0.4f, 0.2f, 0.8f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, -0.5f, 0.5f}, DirectX::XMFLOAT4{0.7f, 0.4f, 0.9f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT4{0.6f, 0.3f, 0.9f, 1.0f}},
        // Top face
        Vertex{DirectX::XMFLOAT3{-0.5f, 0.5f, -0.5f}, DirectX::XMFLOAT4{0.0f, 0.6f, 0.2f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT4{0.1f, 0.7f, 0.3f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{-0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT4{0.0f, 0.8f, 0.4f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{-0.5f, 0.5f, -0.5f}, DirectX::XMFLOAT4{0.0f, 0.6f, 0.2f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, 0.5f, -0.5f}, DirectX::XMFLOAT4{0.1f, 0.7f, 0.25f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT4{0.1f, 0.7f, 0.3f, 1.0f}},
        // Bottom face
        Vertex{DirectX::XMFLOAT3{-0.5f, -0.5f, -0.5f}, DirectX::XMFLOAT4{0.6f, 0.1f, 0.1f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{-0.5f, -0.5f, 0.5f}, DirectX::XMFLOAT4{0.7f, 0.15f, 0.15f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, -0.5f, 0.5f}, DirectX::XMFLOAT4{0.8f, 0.2f, 0.2f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{-0.5f, -0.5f, -0.5f}, DirectX::XMFLOAT4{0.6f, 0.1f, 0.1f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, -0.5f, 0.5f}, DirectX::XMFLOAT4{0.8f, 0.2f, 0.2f, 1.0f}},
        Vertex{DirectX::XMFLOAT3{0.5f, -0.5f, -0.5f}, DirectX::XMFLOAT4{0.7f, 0.15f, 0.15f, 1.0f}},
    };

    constexpr ShaderSource vertexShaderSource{
        .code = "#pragma pack_matrix(row_major)\n"
                "cbuffer TransformCB : register(b0) {\n"
                "    matrix modelMtx;\n"
                "    matrix viewMtx;\n"
                "    matrix projMtx;\n"
                "};\n"
                "struct VSInput { float3 position : POSITION; float4 color : COLOR; };\n"
                "struct PSInput { float4 position : SV_POSITION; float4 color : COLOR; };\n"
                "PSInput main(VSInput input) {\n"
                "    PSInput output;\n"
                "    float4 pos = float4(input.position, 1.0f);\n"
                "    pos = mul(modelMtx, pos);\n"
                "    pos = mul(viewMtx, pos);\n"
                "    pos = mul(projMtx, pos);\n"
                "    output.position = pos;\n"
                "    output.color = input.color;\n"
                "    return output;\n"
                "}\n",
        .entryPoint = "main",
        .target = "vs_5_0",
    };

    constexpr ShaderSource pixelShaderSource{
        .code = "struct PSInput { float4 position : SV_POSITION; float4 color : COLOR; };\n"
                "float4 main(PSInput input) : SV_TARGET { return input.color; }\n",
        .entryPoint = "main",
        .target = "ps_5_0",
    };
}

export class CubeDemo final : public IDemo
{
public:
    CubeDemo(Context& ctx, DXRenderer* pRenderer, std::shared_ptr<IWindow> pWindow);
    ~CubeDemo() override;

    void init() override;
    void update(float dt) override;
    void draw() override;
    void processInput(IWindow::Message /*msg*/, float /*dt*/) override {}

private:
    struct alignas(16) TransformCB
    {
        DirectX::XMMATRIX model;
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX proj;
    };

    DX12Renderer* getDX12Renderer() const;
    void createConstantBuffer();
    void updateConstantBuffer(const DirectX::XMMATRIX& model);

    DrawParams mDrawParams{};
    float mRotationAngle{};
    ComPtr<ID3D12Resource> mpConstantBuffer;
    std::byte* mpMappedConstantBuffer{};
    UINT mConstantBufferSize{};
};

module :private;

CubeDemo::CubeDemo(Context& ctx, DXRenderer* pRenderer, std::shared_ptr<IWindow> pWindow)
    : IDemo{ctx, "Cube Demo", pRenderer, pWindow, nullptr}
{
    ASSERT(mpRenderer != nullptr);
}

CubeDemo::~CubeDemo()
{
    if (mpConstantBuffer != nullptr)
    {
        mpConstantBuffer->Unmap(0u, nullptr);
        mpMappedConstantBuffer = nullptr;
    }

    if (auto* const pDX12Renderer = dynamic_cast<DX12Renderer*>(mpRenderer); pDX12Renderer != nullptr)
    {
        pDX12Renderer->setImmediateConstantBuffer(nullptr);
    }
}

void CubeDemo::init()
{
    GraphicsPipelineDesc pipelineDesc{};
    pipelineDesc.vertexShader = vertexShaderSource;
    pipelineDesc.pixelShader = pixelShaderSource;
    pipelineDesc.inputElements = {
        InputElementDesc{.semanticName = "POSITION", .semanticIndex = 0u, .format = DXGI_FORMAT_R32G32B32_FLOAT,
                         .inputSlot = 0u, .alignedByteOffset = 0u},
        InputElementDesc{.semanticName = "COLOR", .semanticIndex = 0u, .format = DXGI_FORMAT_R32G32B32A32_FLOAT,
                         .inputSlot = 0u, .alignedByteOffset = 12u},
    };
    pipelineDesc.topology = PrimitiveTopology::TriangleList;

    mpRenderer->createPipeline(pipelineDesc);

    VertexBufferDesc vertexBufferDesc{
        .data = std::as_bytes(std::span{cubeVertices}),
        .stride = sizeof(Vertex),
    };

    mpRenderer->createVertexBuffer(vertexBufferDesc);

    mDrawParams.vertexCount = static_cast<uint32_t>(cubeVertices.size());

    createConstantBuffer();
}

void CubeDemo::update(float dt)
{
    IDemo::update(dt);

    mRotationAngle = std::fmod(mRotationAngle + dt, DirectX::XM_2PI);
}

void CubeDemo::draw()
{
    const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationY(mRotationAngle) *
                                       DirectX::XMMatrixRotationX(mRotationAngle * 0.5f);
    const DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(0.0f, 0.0f, 5.0f);

    updateConstantBuffer(translation * rotation);

    auto* const pDX12Renderer = getDX12Renderer();
    ASSERT(pDX12Renderer != nullptr);

    pDX12Renderer->setImmediateConstantBuffer(mpConstantBuffer.Get());
    pDX12Renderer->draw(mDrawParams);
}

DX12Renderer* CubeDemo::getDX12Renderer() const
{
    auto* const pDX12Renderer = dynamic_cast<DX12Renderer*>(mpRenderer);
    ASSERT(pDX12Renderer != nullptr);
    return pDX12Renderer;
}

void CubeDemo::createConstantBuffer()
{
    auto* const pDX12Renderer = getDX12Renderer();
    ID3D12Device* const pDevice = pDX12Renderer->getD3DDevice();
    ASSERT(pDevice != nullptr);

    mConstantBufferSize = (sizeof(TransformCB) + 255u) & ~255u;

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = mConstantBufferSize;
    resourceDesc.Height = 1u;
    resourceDesc.DepthOrArraySize = 1u;
    resourceDesc.MipLevels = 1u;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1u;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HR(pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                        IID_PPV_ARGS(&mpConstantBuffer)));

    D3D12_RANGE readRange{0u, 0u};
    HR(mpConstantBuffer->Map(0u, &readRange, reinterpret_cast<void**>(&mpMappedConstantBuffer)));
    std::memset(mpMappedConstantBuffer, 0, mConstantBufferSize);
}

void CubeDemo::updateConstantBuffer(const DirectX::XMMATRIX& model)
{
    auto* const pWindow = mpWindow.get();
    ASSERT(pWindow != nullptr);

    const DirectX::XMMATRIX view = mCamera.getViewMatrix();
    const float aspectRatio = pWindow->getAspectRatio();
    const DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(
        DirectX::XMConvertToRadians(mCamera.getZoom()), aspectRatio, 0.1f, 100.0f);

    TransformCB cb{};
    cb.model = model;
    cb.view = view;
    cb.proj = proj;

    ASSERT(sizeof(TransformCB) <= mConstantBufferSize);
    std::memcpy(mpMappedConstantBuffer, &cb, sizeof(TransformCB));
}
