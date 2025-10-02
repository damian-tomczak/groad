module;

#include "utils.h"

#include <DirectXMath.h>

#include <array>
#include <cmath>
#include <span>

export module cube_demo;

export import core.demo;
import dx.render_backend;
import dxrenderer;

namespace
{
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
        .code = "matrix modelMtx;\n"
                "matrix viewMtx;\n"
                "matrix invViewMtx;\n"
                "matrix projMtx;\n"
                "matrix invProjMtx;\n"
                "matrix texMtx;\n"
                "float4 cameraPos;\n"
                "float4 color;\n"
                "int flags;\n"
                "int screenWidth;\n"
                "int screenHeight;\n"
                "int tesFactor;\n"
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

    void init() override;
    void update(float dt) override;
    void draw() override;
    void processInput(IWindow::Message /*msg*/, float /*dt*/) override {}

private:
    DrawParams mDrawParams{};
    float mRotationAngle{};
};

module :private;

CubeDemo::CubeDemo(Context& ctx, DXRenderer* pRenderer, std::shared_ptr<IWindow> pWindow)
    : IDemo{ctx, "Cube Demo", pRenderer, pWindow, nullptr}
{
    ASSERT(mpRenderer != nullptr);
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
    const DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation(0.0f, 0.0f, 15.0f);

    mpRenderer->mGlobalCB.modelMtx = rotation * translation;
    mpRenderer->updateCB(mpRenderer->mGlobalCB);

    mpRenderer->draw(mDrawParams);
}
