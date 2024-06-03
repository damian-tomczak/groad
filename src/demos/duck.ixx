module;
#include "utils.h"

export module duck_demo;
export import demo;

struct VertexPositionNormal
{
    struct Data
    {
        XMFLOAT3 position;
        XMFLOAT3 normal;
    } data;

    inline const static std::array<D3D11_INPUT_ELEMENT_DESC, 2> layout{{
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    }};

    static constexpr unsigned int stride = sizeof(Data);
    static constexpr unsigned int offset = 0;
};

class Mesh : public NonCopyable
{
public:
    Mesh(IRenderer* pRenderer, std::pair<std::vector<VertexPositionNormal>, std::vector<short>>&& data)
        : mpRenderer{pRenderer}, mVertices{std::move(data.first)}, mIndices{std::move(data.second)}
    {
    }

    void init()
    {
        DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it

        HR(pDX11Renderer->getDevice()->CreateInputLayout(
            VertexPositionNormal::layout.data(),
            static_cast<unsigned int>(VertexPositionNormal::layout.size()),
            pDX11Renderer->getShaders().wallsVS.second->GetBufferPointer(),
            pDX11Renderer->getShaders().wallsVS.second->GetBufferSize(),
            mpInputLayout.GetAddressOf()));

        const D3D11_BUFFER_DESC vbd{
            .ByteWidth = static_cast<unsigned int>(sizeof(VertexPositionNormal) * mVertices.size()),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = 0,
            .StructureByteStride = 0,
        };
        const D3D11_SUBRESOURCE_DATA vinitData{
            .pSysMem = mVertices.data(),
        };
        HR(pDX11Renderer->getDevice()->CreateBuffer(&vbd, &vinitData, &mpVertexBuffer));

        const D3D11_BUFFER_DESC ibd{
            .ByteWidth = static_cast<unsigned int>(sizeof(short) * mIndices.size()),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_INDEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = 0,
            .StructureByteStride = 0,
        };
        const D3D11_SUBRESOURCE_DATA initData{
            .pSysMem = mIndices.data(),
        };
        HR(pDX11Renderer->getDevice()->CreateBuffer(&ibd, &initData, &mpIndexBuffer));

        const D3D11_RASTERIZER_DESC restarizerDesc{
            .FillMode = D3D11_FILL_SOLID,
            .CullMode = D3D11_CULL_BACK,
            .FrontCounterClockwise = false,
            .DepthBias = 0,
            .DepthBiasClamp = 0.0f,
            .SlopeScaledDepthBias = 0.0f,
            .DepthClipEnable = true,
            .ScissorEnable = false,
            .MultisampleEnable = false,
            .AntialiasedLineEnable = false,
        };

        HR(pDX11Renderer->getDevice()->CreateRasterizerState(&restarizerDesc, mpRasterizer.GetAddressOf()));
    }

    void draw()
    {
        DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it

        pDX11Renderer->getContext()->RSSetState(mpRasterizer.Get());

        pDX11Renderer->getContext()->IASetVertexBuffers(
            0,
            1,
            mpVertexBuffer.GetAddressOf(),
            &VertexPositionNormal::stride,
            &VertexPositionNormal::offset);

        pDX11Renderer->getContext()->IASetIndexBuffer(mpIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);


        pDX11Renderer->getContext()->IASetInputLayout(mpInputLayout.Get());
        pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        pDX11Renderer->getContext()->DrawIndexed(static_cast<unsigned int>(mIndices.size()), 0, 0);
    }

private:
    ComPtr<ID3D11Buffer> mpIndexBuffer;
    ComPtr<ID3D11Buffer> mpVertexBuffer;
    ComPtr<ID3D11InputLayout> mpInputLayout;
    ComPtr<ID3D11RasterizerState> mpRasterizer;

    std::vector<VertexPositionNormal> mVertices;
    std::vector<short> mIndices;

    IRenderer* mpRenderer;
};

export class DuckDemo : public IDemo
{
    static constexpr float cDropTime = 0.05f;

public:
    DuckDemo(IRenderer* pRenderer);

    void init() override;
    void update(float dt) override;
    void draw(GlobalCB& cb) override;
    void processInput(IWindow::Message msg, float dt) override;

private:
    void drawDuck();
    void drawBox();

private:
    Mesh mBox;
};

module :private;

// clang-format off

std::vector<VertexPositionNormal> boxVertices(float width, float height, float depth)
{
    return
    {
        //Front face
        { XMFLOAT3{-0.5f*width, -0.5f*height, -0.5f*depth}, XMFLOAT3{0.0f, 0.0f, 1.0f} },
        { XMFLOAT3{+0.5f*width, -0.5f*height, -0.5f*depth}, XMFLOAT3{0.0f, 0.0f, 1.0f} },
        { XMFLOAT3{+0.5f*width, +0.5f*height, -0.5f*depth}, XMFLOAT3{0.0f, 0.0f, 1.0f} },
        { XMFLOAT3{-0.5f*width, +0.5f*height, -0.5f*depth}, XMFLOAT3{0.0f, 0.0f, 1.0f} },

        //Back face
        { XMFLOAT3{+0.5f*width, -0.5f*height, +0.5f*depth}, XMFLOAT3{0.0f, 0.0f, -1.0f} },
        { XMFLOAT3{-0.5f*width, -0.5f*height, +0.5f*depth}, XMFLOAT3{0.0f, 0.0f, -1.0f} },
        { XMFLOAT3{-0.5f*width, +0.5f*height, +0.5f*depth}, XMFLOAT3{0.0f, 0.0f, -1.0f} },
        { XMFLOAT3{+0.5f*width, +0.5f*height, +0.5f*depth}, XMFLOAT3{0.0f, 0.0f, -1.0f} },

        //Left face
        { XMFLOAT3{-0.5f*width, -0.5f*height, +0.5f*depth}, XMFLOAT3{1.0f, 0.0f, 0.0f} },
        { XMFLOAT3{-0.5f*width, -0.5f*height, -0.5f*depth}, XMFLOAT3{1.0f, 0.0f, 0.0f} },
        { XMFLOAT3{-0.5f*width, +0.5f*height, -0.5f*depth}, XMFLOAT3{1.0f, 0.0f, 0.0f} },
        { XMFLOAT3{-0.5f*width, +0.5f*height, +0.5f*depth}, XMFLOAT3{1.0f, 0.0f, 0.0f} },

        //Right face
        { XMFLOAT3{+0.5f*width, -0.5f*height, -0.5f*depth}, XMFLOAT3{-1.0f, 0.0f, 0.0f} },
        { XMFLOAT3{+0.5f*width, -0.5f*height, +0.5f*depth}, XMFLOAT3{-1.0f, 0.0f, 0.0f} },
        { XMFLOAT3{+0.5f*width, +0.5f*height, +0.5f*depth}, XMFLOAT3{-1.0f, 0.0f, 0.0f} },
        { XMFLOAT3{+0.5f*width, +0.5f*height, -0.5f*depth}, XMFLOAT3{-1.0f, 0.0f, 0.0f} },

        //Bottom face
        { XMFLOAT3{-0.5f*width, -0.5f*height, +0.5f*depth}, XMFLOAT3{0.0f, 1.0f, 0.0f} },
        { XMFLOAT3{+0.5f*width, -0.5f*height, +0.5f*depth}, XMFLOAT3{0.0f, 1.0f, 0.0f} },
        { XMFLOAT3{+0.5f*width, -0.5f*height, -0.5f*depth}, XMFLOAT3{0.0f, 1.0f, 0.0f} },
        { XMFLOAT3{-0.5f*width, -0.5f*height, -0.5f*depth}, XMFLOAT3{0.0f, 1.0f, 0.0f} },

        //Top face
        { XMFLOAT3{-0.5f*width, +0.5f*height, -0.5f*depth}, XMFLOAT3{0.0f, -1.0f, 0.0f} },
        { XMFLOAT3{+0.5f*width, +0.5f*height, -0.5f*depth}, XMFLOAT3{0.0f, -1.0f, 0.0f} },
        { XMFLOAT3{+0.5f*width, +0.5f*height, +0.5f*depth}, XMFLOAT3{0.0f, -1.0f, 0.0f} },
        { XMFLOAT3{-0.5f*width, +0.5f*height, +0.5f*depth}, XMFLOAT3{0.0f, -1.0f, 0.0f} },
    };
}

std::vector<short> boxIndices()
{
    return
    {
        0, 2, 1,  0, 3, 2,
        4, 6, 5,  4, 7, 6,
        8, 10,9,  8, 11,10,
        12,14,13, 12,15,14,
        16,18,17, 16,19,18,
        20,22,21, 20,23,22,
    };
}

// clang-format on

std::pair<std::vector<VertexPositionNormal>, std::vector<short>> createBox(float width, float height, float depth)
{
    std::vector<short> indices = boxIndices();
    std::reverse(indices.begin(), indices.end());
    return {boxVertices(width, height, depth), indices};
}

DuckDemo::DuckDemo(IRenderer* pRenderer) : IDemo{"DuckDemo", pRenderer}, mBox(mpRenderer, createBox(9.90f, 10.0f, 9.90f))
{
    mpSurface = std::make_unique<WaterSurface>(mpRenderer, 10.0f, 256);
}

void DuckDemo::init()
{
    IDemo::init();

    mBox.init();

    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it
}

void DuckDemo::draw(GlobalCB& cb)
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it
    ID3D11DeviceContext* const pContext = pDX11Renderer->getContext();

    drawDuck();
    drawBox();
    mpSurface->draw(cb);
}

void DuckDemo::processInput(IWindow::Message msg, float dt)
{

}

void DuckDemo::update(float dt)
{
    IDemo::update(dt);
}

void DuckDemo::drawDuck()
{

}

void DuckDemo::drawBox()
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it

    pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().wallsVS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().wallsPS.first.Get(), nullptr, 0);

    mBox.draw();
}