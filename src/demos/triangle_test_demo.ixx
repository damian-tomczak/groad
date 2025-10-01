module;

#include "utils.h"

#include <DirectXMath.h>
#include <array>
#include <span>

export module triangle_test_demo;

export import core.demo;
import dxrenderer;
import dx.render_backend;

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

    constexpr ShaderSource vertexShaderSource{
        .code = "struct VSInput { float3 position : POSITION; float4 color : COLOR; };\n"
                "struct PSInput { float4 position : SV_POSITION; float4 color : COLOR; };\n"
                "PSInput main(VSInput input) { PSInput output;\n"
                "    output.position = float4(input.position, 1.0f);\n"
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

export class TriangleTestDemo final : public IDemo
{
public:
    TriangleTestDemo(Context& ctx, DXRenderer* pRenderer, std::shared_ptr<IWindow> pWindow);

    void init() override;
    void draw() override;
    void processInput(IWindow::Message /*msg*/, float /*dt*/) override {}
    void update(float dt) override { IDemo::update(dt); }

private:
    std::unique_ptr<IRenderBackend> mpRenderBackend;
    DrawParams mDrawParams{};
};

module :private;

TriangleTestDemo::TriangleTestDemo(Context& ctx, DXRenderer* pRenderer, std::shared_ptr<IWindow> pWindow)
    : IDemo{ctx, "Triangle Test", pRenderer, pWindow, nullptr}
    , mpRenderBackend{pRenderer != nullptr ? pRenderer->createRenderBackend() : nullptr}
{
    ASSERT(mpRenderBackend != nullptr);
}

void TriangleTestDemo::init()
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

    mpRenderBackend->createPipeline(pipelineDesc);

    VertexBufferDesc vertexBufferDesc{
        .data = std::as_bytes(std::span{triangleVertices}),
        .stride = sizeof(Vertex),
    };

    mpRenderBackend->createVertexBuffer(vertexBufferDesc);

    mDrawParams.vertexCount = static_cast<uint32_t>(triangleVertices.size());
}

void TriangleTestDemo::draw()
{
    mpRenderBackend->draw(mDrawParams);
}
