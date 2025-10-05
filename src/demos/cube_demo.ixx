module;

#include "utils.h"

#include <DirectXMath.h>

#include <d3d11.h>
#include <d3d12.h>
#include <wrl/client.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <span>

export module cube_demo;

export import core.demo;
import dx11renderer;
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

    static_assert(sizeof(TransformCB) % 16u == 0u, "Transform constant buffer must be 16-byte aligned.");

    void createConstantBuffer();
    void updateConstantBuffer(const DirectX::XMMATRIX& model);

    DrawParams mDrawParams{};
    float mRotationAngle{};
    ComPtr<ID3D11Buffer> mpDX11ConstantBuffer;
    ComPtr<ID3D12Resource> mpDX12ConstantBuffer;
    std::byte* mpDX12MappedConstantBuffer{};
    UINT mDX12ConstantBufferSize{};
    DX11Renderer* mpDX11Renderer{};
    DX12Renderer* mpDX12Renderer{};
};

module :private;

CubeDemo::CubeDemo(Context& ctx, DXRenderer* pRenderer, std::shared_ptr<IWindow> pWindow)
    : IDemo{ctx, "Cube Demo", pRenderer, pWindow, nullptr}
    , mpDX11Renderer{dynamic_cast<DX11Renderer*>(pRenderer)}
    , mpDX12Renderer{dynamic_cast<DX12Renderer*>(pRenderer)}
{
    ASSERT(mpRenderer != nullptr);
}

CubeDemo::~CubeDemo()
{
    if (mpDX12ConstantBuffer != nullptr)
    {
        mpDX12ConstantBuffer->Unmap(0u, nullptr);
        mpDX12MappedConstantBuffer = nullptr;
    }

    if (mpDX12Renderer != nullptr)
    {
        mpDX12Renderer->setImmediateConstantBuffer(nullptr);
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
    if (mpRenderer == nullptr)
    {
        return;
    }

    const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationY(mRotationAngle) *
                                       DirectX::XMMatrixRotationX(mRotationAngle * 0.5f);
    const DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(0.0f, 0.0f, 15.0f);

    updateConstantBuffer(translation * rotation);

    if (mpDX12Renderer != nullptr)
    {
        mpDX12Renderer->setImmediateConstantBuffer(mpDX12ConstantBuffer.Get());
    }
    else if (mpDX11Renderer != nullptr)
    {
        ID3D11DeviceContext* const pContext = mpDX11Renderer->getContext();
        ASSERT(pContext != nullptr);

        ID3D11Buffer* const pConstantBuffer = mpDX11ConstantBuffer.Get();
        ASSERT(pConstantBuffer != nullptr);

        pContext->VSSetConstantBuffers(0u, 1u, &pConstantBuffer);
    }

    mpRenderer->draw(mDrawParams);

    const DirectX::XMMATRIX innerRotation = DirectX::XMMatrixRotationY(-mRotationAngle * 1.5f) *
                                            DirectX::XMMatrixRotationX(-mRotationAngle * 0.75f);
    const DirectX::XMMATRIX innerScale = DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f);

    updateConstantBuffer(translation * innerRotation * innerScale);

    mpRenderer->draw(mDrawParams);
}

void CubeDemo::createConstantBuffer()
{
    if (mpDX12Renderer != nullptr)
    {
        ID3D12Device* const pDevice = mpDX12Renderer->getD3DDevice();
        ASSERT(pDevice != nullptr);

        mDX12ConstantBufferSize = (sizeof(TransformCB) + 255u) & ~255u;

        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = mDX12ConstantBufferSize;
        resourceDesc.Height = 1u;
        resourceDesc.DepthOrArraySize = 1u;
        resourceDesc.MipLevels = 1u;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1u;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HR(pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
                                            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                            IID_PPV_ARGS(&mpDX12ConstantBuffer)));

        D3D12_RANGE readRange{0u, 0u};
        HR(mpDX12ConstantBuffer->Map(0u, &readRange,
                                     reinterpret_cast<void**>(&mpDX12MappedConstantBuffer)));
        std::memset(mpDX12MappedConstantBuffer, 0, mDX12ConstantBufferSize);
    }

    if (mpDX11Renderer != nullptr)
    {
        ID3D11Device* const pDevice = mpDX11Renderer->getDevice();
        ASSERT(pDevice != nullptr);

        const UINT constantBufferSize = (sizeof(TransformCB) + 15u) & ~15u;

        TransformCB initialData{};

        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = constantBufferSize;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA subresourceData{};
        subresourceData.pSysMem = &initialData;

        HR(pDevice->CreateBuffer(&desc, &subresourceData, mpDX11ConstantBuffer.GetAddressOf()));
    }
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

    if (mpDX12MappedConstantBuffer != nullptr)
    {
        ASSERT(sizeof(TransformCB) <= mDX12ConstantBufferSize);
        std::memcpy(mpDX12MappedConstantBuffer, &cb, sizeof(TransformCB));
    }

    if (mpDX11ConstantBuffer != nullptr)
    {
        ID3D11DeviceContext* const pContext = mpDX11Renderer->getContext();
        ASSERT(pContext != nullptr);

        D3D11_MAPPED_SUBRESOURCE mappedResource{};
        HR(pContext->Map(mpDX11ConstantBuffer.Get(), 0u, D3D11_MAP_WRITE_DISCARD, 0u, &mappedResource));
        std::memcpy(mappedResource.pData, &cb, sizeof(TransformCB));
        pContext->Unmap(mpDX11ConstantBuffer.Get(), 0u);
    }
}
