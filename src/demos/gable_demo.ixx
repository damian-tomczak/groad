module;
#include "utils.h"

#include "imgui.h"

export module gable_demo;
export import core.demo;
import dxrenderer;

export class GableDemo : public IDemo
{
public:
    GableDemo(Context& ctx, DXRenderer* pRenderer, std::shared_ptr<IWindow> pWindow);

    void init() override;
    void update(float dt) override;
    void draw() override;
    void processInput(IWindow::Message msg, float dt) override;
    void renderUi() override;

private:
    struct TesselationFactors : public CB<ID3D11Buffer>
    {
        float inside = 6.f;
        float outside = 6.f;
        bool isWireframe = false;
        char padding[7];
    } mTesselationFactors{};

    std::vector<DirectX::XMFLOAT3> mVertices1, mVertices2;
    ComPtr<ID3D11Buffer> mpVertexBuffer1, mpVertexBuffer2;
    ID3D11Buffer* mpCurrentVertexBuffer{};
    ComPtr<ID3D11Buffer> mpIndexBuffer1, mpIndexBuffer2;
    ComPtr<ID3D11RasterizerState> mpRasterizerState;
};

module :private;

GableDemo::GableDemo(Context& ctx, DXRenderer* pRenderer, std::shared_ptr<IWindow> pWindow)
    : IDemo{ctx, "GableDemo", pRenderer, pWindow, nullptr}
{
}

void GableDemo::init()
{
    IDemo::init();

    auto* pDXRenderer = mpRenderer;
    ASSERT(pDXRenderer != nullptr);

	float gap = 1.f;
    float x = -1.5f * gap;
    for (size_t i = 0; i < 4; i++)
    {
        float y = -1.5f * gap;

        mVertices2.push_back(XMFLOAT3(x, y, 0.f));
        mVertices1.push_back(XMFLOAT3(x, y, 0.f));
        y += gap;

        mVertices2.push_back(XMFLOAT3(x, y, -1.f));
        mVertices1.push_back(XMFLOAT3(x, y, 1.f));
        y += gap;

        mVertices2.push_back(XMFLOAT3(x, y, 1.f));
        mVertices1.push_back(XMFLOAT3(x, y, 1.f));
        y += gap;

        mVertices2.push_back(XMFLOAT3(x, y, 0.f));
        mVertices1.push_back(XMFLOAT3(x, y, 0.f));

        y += gap;
        x += gap;
    }

    const std::vector<short> indices
    {
        0,  4,  4,  5,  5,  1, 1, 0, // 8
        5,  6,  6,  2,  2,  1,       // 6
        6,  7,  7,  3,  3,  2,       // 6
        4,  8,  8,  9,  9,  5,       // 6
        9,  10, 10, 6,               // 4
        10, 11, 11, 7,               // 4
        8,  12, 12, 13, 13, 9,       // 6
        13, 14, 14, 10,              // 4
        14, 15, 15, 11               // 4
    };                               // 48

    D3D11_BUFFER_DESC vbd{
        .ByteWidth = static_cast<UINT>(sizeof(XMFLOAT3) * mVertices1.size()),
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0,
        .StructureByteStride = 0,
    };
    D3D11_SUBRESOURCE_DATA initData{
        .pSysMem = mVertices1.data(),
    };

    HR(pDXRenderer->getDevice()->CreateBuffer(&vbd, &initData, mpVertexBuffer1.GetAddressOf()));
    mpCurrentVertexBuffer = mpVertexBuffer1.Get();
    initData.pSysMem = mVertices2.data();

    HR(pDXRenderer->getDevice()->CreateBuffer(&vbd, &initData, mpVertexBuffer2.GetAddressOf()));

    D3D11_BUFFER_DESC ibd1{
        .ByteWidth = static_cast<UINT>(sizeof(unsigned) * indices.size()),
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_INDEX_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0,
        .StructureByteStride = 0,
    };

    initData.pSysMem = indices.data();

    HR(pDXRenderer->getDevice()->CreateBuffer(&ibd1, &initData, mpIndexBuffer1.GetAddressOf()));

    std::vector<short> indices2
    {
        0, 1, 2, 3,
        4, 5, 6, 7,
        8, 9, 10, 11,
        12, 13, 14, 15
    };

    D3D11_BUFFER_DESC ibd2{
        .ByteWidth = static_cast<UINT>(sizeof(short) * indices2.size()),
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_INDEX_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0,
        .StructureByteStride = 0,
    };

    initData.pSysMem = indices2.data();

    HR(pDXRenderer->getDevice()->CreateBuffer(&ibd2, &initData, mpIndexBuffer2.GetAddressOf()));

    pDXRenderer->createCB(mTesselationFactors);
}

void GableDemo::draw()
{
    IDemo::draw();

    auto* pDXRenderer = mpRenderer;
    ASSERT(pDXRenderer != nullptr);

    // Border
    pDXRenderer->getContext()->VSSetShader(pDXRenderer->getShaders().gableBorderVS.first.Get(), nullptr, 0);
    pDXRenderer->getContext()->PSSetShader(pDXRenderer->getShaders().gableBorderPS.first.Get(), nullptr, 0);

    unsigned int vertexStride = sizeof(XMFLOAT3), offset = 0;
    pDXRenderer->getContext()->IASetVertexBuffers(0, 1, &mpCurrentVertexBuffer, &vertexStride, &offset);
    pDXRenderer->getContext()->IASetIndexBuffer(mpIndexBuffer1.Get(), DXGI_FORMAT_R16_UINT, 0);
    pDXRenderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    pDXRenderer->getContext()->DrawIndexed(48, 0, 0);

    const D3D11_RASTERIZER_DESC rasterDesc{
        .FillMode = mTesselationFactors.isWireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID,
        .CullMode = D3D11_CULL_NONE,
        .FrontCounterClockwise = false,
        .DepthBias = 0,
        .DepthBiasClamp = 0.0f,
        .SlopeScaledDepthBias = 0.0f,
        .DepthClipEnable = true,
        .ScissorEnable = false,
        .MultisampleEnable = false,
        .AntialiasedLineEnable = false,
    };

    HR(pDXRenderer->getDevice()->CreateRasterizerState(&rasterDesc, &mpRasterizerState));

    pDXRenderer->getContext()->RSSetState(mpRasterizerState.Get());

    //
    pDXRenderer->getContext()->VSSetShader(pDXRenderer->getShaders().gableVS.first.Get(), nullptr, 0);
    pDXRenderer->getContext()->HSSetShader(pDXRenderer->getShaders().gableHS.first.Get(), nullptr, 0);
    pDXRenderer->getContext()->DSSetShader(pDXRenderer->getShaders().gableDS.first.Get(), nullptr, 0);
    pDXRenderer->getContext()->PSSetShader(pDXRenderer->getShaders().gablePS.first.Get(), nullptr, 0);

    pDXRenderer->getContext()->HSSetConstantBuffers(2, 1, mTesselationFactors.buffer.GetAddressOf());
    pDXRenderer->getContext()->PSSetConstantBuffers(2, 1, mTesselationFactors.buffer.GetAddressOf());


    pDXRenderer->getContext()->IASetIndexBuffer(mpIndexBuffer2.Get(), DXGI_FORMAT_R16_UINT, 0);
    pDXRenderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST);
    pDXRenderer->getContext()->DrawIndexed(16, 0, 0);
}

void GableDemo::processInput(IWindow::Message msg, float dt)
{
    switch (msg)
    {
    case IWindow::Message::KEY_E_DOWN:
        mpCurrentVertexBuffer = (mpCurrentVertexBuffer == mpVertexBuffer2.Get()) ? mpVertexBuffer1.Get() : mpVertexBuffer2.Get();
    }
}

void GableDemo::renderUi()
{
    auto* pDXRenderer = mpRenderer;
    ASSERT(pDXRenderer != nullptr);

    ImGui::Begin("Control Panel");

    ImGui::SliderFloat("Inside", &mTesselationFactors.inside, 0.0f, 100.0f);

    ImGui::SliderFloat("Outside", &mTesselationFactors.outside, 0.0f, 100.0f);

    ImGui::Checkbox("Show Wireframe", &mTesselationFactors.isWireframe);

    pDXRenderer->updateCB(mTesselationFactors);

    ImGui::End();
}

void GableDemo::update(float dt)
{
    IDemo::update(dt);
}

