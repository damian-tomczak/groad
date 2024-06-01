module;

#include "utils.h"

export module surface;
import dx11renderer;
import core;

export class ISurface
{
public:
    virtual void init()
    {

    }
    virtual void update(float dt) = 0;
    virtual void draw(GlobalCB& cb) = 0;

protected:
    ISurface(IRenderer* pRenderer) : mpRenderer{pRenderer}
    {

    }

    IRenderer* mpRenderer;
};

export class WaterSurface : public ISurface
{
    static constexpr UINT stride = sizeof(XMFLOAT3);
    static constexpr UINT offset = 0;

public:
    WaterSurface(IRenderer* pRenderer, float planeWidth, float planeHeight, int samplesTextureWidth,
                 int samplesTextureHeight);

    void init() override;
    void draw(GlobalCB& cb) override;

    void applyDisturbanceInWorldSpace(XMFLOAT3 position, float strength);
    void update(float deltaTime) override;

protected:
    void clearSamples();
    void calculateNormalMap();
    std::vector<unsigned char> buildNormalMap();
    void copyNormalsToTexture(std::vector<unsigned char>& normals);

private:
    ComPtr<ID3D11Buffer> mpVertexBuffer, mpIndexBuffer;
    ComPtr<ID3D11ShaderResourceView> mCubeMap;

    float mPlaneWidth, mPlaneHeight;
    unsigned mSamplesTextureWidth, mSamplesTextureHeight;
    std::vector<float> _samples, _samples2;
    std::vector<XMFLOAT3> _normals;
    ComPtr<ID3D11ShaderResourceView> mpNormalMapTexture;

    std::vector<float>*_currentSamples{}, *_previousSamples{};

    XMMATRIX _modelMatrix, _invModelMatrix, mTexMtx;

    ComPtr<ID3D11InputLayout> mpInputLayout{};
    ComPtr<ID3D11SamplerState> mpSamplerState{};
};

export class GridSurface : public ISurface
{
public:
    GridSurface(IRenderer* pRenderer) : ISurface{pRenderer}
    {

    }

    void init() override;
    void draw(GlobalCB& cb) override
    {
        DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it
        ID3D11DeviceContext* const pContext = pDX11Renderer->getContext();

        pContext->VSSetShader(pDX11Renderer->getShaders().pGridVS.first.Get(), nullptr, 0);
        pContext->PSSetShader(pDX11Renderer->getShaders().pGridPS.first.Get(), nullptr, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pContext->Draw(6, 0);
    }

    void update(float dt) override
    {

    }
};

module :private;

void createPlane(ID3D11Device* pDevice, float width, float length, ID3D11Buffer** pVertexBuffer, ID3D11Buffer** pIndexBuffer)
{
    float halfWidth = 0.5f * width;
    float halfLength = 0.5f * length;

    const XMFLOAT3 vertices[]{
        {-halfWidth, 0.0f, +halfLength},
        {+halfWidth, 0.0f, +halfLength},
        {-halfWidth, 0.0f, -halfLength},
        {+halfWidth, 0.0f, -halfLength},
    };

    UINT indices[]{0, 1, 2, 1, 3, 2};

    D3D11_BUFFER_DESC bufferDesc;
    D3D11_SUBRESOURCE_DATA initData;

    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;
    initData.pSysMem = vertices;

    HR(pDevice->CreateBuffer(&bufferDesc, &initData, pVertexBuffer));

    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(indices);
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;
    initData.pSysMem = indices;

    HR(pDevice->CreateBuffer(&bufferDesc, &initData, pIndexBuffer));
}

WaterSurface::WaterSurface(IRenderer* pRenderer, float planeWidth, float planeHeight, int samplesTextureWidth, int samplesTextureHeight)
    : ISurface{pRenderer}, mPlaneWidth(planeWidth), mPlaneHeight(planeHeight),
      mSamplesTextureWidth(samplesTextureWidth), mSamplesTextureHeight(samplesTextureHeight)
{
    _modelMatrix = XMMatrixIdentity();
    _invModelMatrix = XMMatrixInverse(nullptr, _modelMatrix);
}

void WaterSurface::applyDisturbanceInWorldSpace(XMFLOAT3 position, float strength)
{
    XMVECTOR posVec = XMLoadFloat3(&position);
    posVec = XMVectorSetW(posVec, 1.0f);

    XMMATRIX invModelMatrix = XMMatrixInverse(nullptr, _modelMatrix);
    XMVECTOR textureSpacePositionVec = XMVector3TransformCoord(posVec, invModelMatrix);
    textureSpacePositionVec = XMVector3TransformCoord(textureSpacePositionVec, mTexMtx);

    XMFLOAT3 textureSpacePosition;
    XMStoreFloat3(&textureSpacePosition, textureSpacePositionVec);

    if (textureSpacePosition.x < 0.0f || textureSpacePosition.x > 1.0f || textureSpacePosition.z < 0.0f ||
        textureSpacePosition.z > 1.0f)
    {
        return;
    }

    int coordX = static_cast<int>(textureSpacePosition.x * mSamplesTextureWidth);
    int coordY = static_cast<int>(textureSpacePosition.z * mSamplesTextureHeight);

    (*_currentSamples)[coordY * mSamplesTextureWidth + coordX] += strength;
}

void WaterSurface::update(float deltaTime)
{
    int N = 256;
    float h = 2.0f / (N - 1);
    float c = 1.0f;
    float dt = 1.0f / N;

    float A = (c * c) * (dt * dt) / (h * h);
    float B = 2 - 4 * A;

    for (unsigned y = 0; y < mSamplesTextureHeight; ++y)
    {
        for (unsigned x = 0; x < mSamplesTextureWidth; ++x)
        {
            int baseIndex = y * mSamplesTextureWidth + x;
            float previous = (*_previousSamples)[baseIndex];
            float current = (*_currentSamples)[baseIndex];
            float neighborsSum = 0.0f;
            int dispX[] = {-1, 1, 0, 0};
            int dispY[] = {0, 0, -1, 1};

            for (int i = 0; i < 4; ++i)
            {
                int fx = x + dispX[i];
                int fy = y + dispY[i];
                if (fx >= 0 && fx < static_cast<int>(mSamplesTextureWidth) && fy >= 0 && fy < static_cast<int>(mSamplesTextureHeight))
                {
                    neighborsSum += (*_currentSamples)[fy * mSamplesTextureWidth + fx];
                }
            }

            float px = static_cast<float>(x) / (mSamplesTextureWidth - 1);
            float py = static_cast<float>(y) / (mSamplesTextureHeight - 1);
            float l = std::min(px, std::min(1.0f - px, std::min(py, 1.0f - py)));
            float damping = 0.95f * std::min(1.0f, l / 0.01f);

            (*_previousSamples)[baseIndex] = damping * (A * neighborsSum + B * current - previous);
        }
    }

    swap(_currentSamples, _previousSamples);

    calculateNormalMap();
}

void WaterSurface::init()
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it

    clearSamples();

    const D3D11_TEXTURE2D_DESC texDesc{
        .Width = mSamplesTextureWidth,
        .Height = mSamplesTextureHeight,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc{
            .Count = 1,
            .Quality = 0,
        },
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags = 0,
    };

    std::vector<unsigned char> normalMapData = buildNormalMap();

    const D3D11_SUBRESOURCE_DATA initData{
        .pSysMem = normalMapData.data(),
        .SysMemPitch = mSamplesTextureWidth * 3,
    };

    ID3D11Texture2D* pTexture{};
    HR(pDX11Renderer->getDevice()->CreateTexture2D(&texDesc, &initData, &pTexture));

    HR(pDX11Renderer->getDevice()->CreateShaderResourceView(pTexture, nullptr, &mpNormalMapTexture));
    pTexture->Release();

    createPlane(pDX11Renderer->getDevice(), mPlaneWidth, mPlaneHeight, mpVertexBuffer.GetAddressOf(), mpIndexBuffer.GetAddressOf());

    mTexMtx = XMMatrixScaling(1.0f / mPlaneWidth, 0, 1.0f / mPlaneHeight);
    mTexMtx = XMMatrixTranslation(0.5f * mPlaneWidth, 0, 0.5f * mPlaneHeight) * mTexMtx;

    std::array des{
        D3D11_INPUT_ELEMENT_DESC{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    HR(pDX11Renderer->getDevice()->CreateInputLayout(
        des.data(), static_cast<UINT>(des.size()),
                                   pDX11Renderer->getShaders().pDefaultVS.second->GetBufferPointer(),
                                   pDX11Renderer->getShaders().pDefaultVS.second->GetBufferSize(),
                                   &mpInputLayout));

    const D3D11_SAMPLER_DESC sampDesc{
        .Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
        .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
        .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
        .ComparisonFunc = D3D11_COMPARISON_NEVER,
        .MinLOD = 0,
        .MaxLOD = D3D11_FLOAT32_MAX,
    };

    HR(pDX11Renderer->getDevice()->CreateSamplerState(&sampDesc, &mpSamplerState));
}

void WaterSurface::draw(GlobalCB& cb)
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it

    auto normalMapData = buildNormalMap();
    copyNormalsToTexture(normalMapData);

    pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().pWaterSurfaceVS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().pWaterSurfacePS.first.Get(), nullptr, 0);

    // TODO: investigate
    auto var = mpNormalMapTexture;
    pDX11Renderer->getContext()->PSSetShaderResources(0, 1, &mpNormalMapTexture);
    pDX11Renderer->getContext()->PSSetShaderResources(1, 1, &mCubeMap);

    ID3D11SamplerState* pSamplers[]{mpSamplerState.Get(), mpSamplerState.Get()};
    pDX11Renderer->getContext()->PSSetSamplers(0, 2, pSamplers);

    pDX11Renderer->getContext()->IASetVertexBuffers(0, 1, mpVertexBuffer.GetAddressOf(), &stride, &offset);
    pDX11Renderer->getContext()->IASetIndexBuffer(mpIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pDX11Renderer->getContext()->DrawIndexed(6, 0, 0);
}

void WaterSurface::clearSamples()
{
    unsigned totalSamples = mSamplesTextureWidth * mSamplesTextureHeight;
    _samples.resize(totalSamples);
    _samples2.resize(totalSamples);
    _normals.resize(totalSamples);

    _currentSamples = &_samples;
    _previousSamples = &_samples2;

    for (unsigned i = 0; i < totalSamples; ++i)
    {
        _samples[i] = 0.0f;
        _samples2[i] = 0.0f;
        _normals[i] = XMFLOAT3(0.0f, 1.0f, 0.0f);
    }
}

void WaterSurface::calculateNormalMap()
{
    for (unsigned y = 0; y < mSamplesTextureHeight; ++y)
    {
        for (unsigned x = 0; x < mSamplesTextureWidth; ++x)
        {
            int baseIndex = y * mSamplesTextureWidth + x;
            _normals[baseIndex] = XMFLOAT3(0.0f, 0.0f, 0.0f);
        }
    }

    for (unsigned y = 0; y < mSamplesTextureHeight; ++y)
    {
        for (unsigned x = 0; x < mSamplesTextureWidth; ++x)
        {
            int baseIndex = y * mSamplesTextureWidth + x;

            int dispX[] = {-1, 1};
            int dispY[] = {-1, 1};
            XMVECTOR sum{};

            for (int i = 0; i < 2; ++i)
            {
                int fx = x + dispX[i];
                int fy = y + dispY[i];
                if (fx < 0 || fx >= static_cast<int>(mSamplesTextureWidth) || fy < 0 || fy >= static_cast<int>(mSamplesTextureHeight))
                {
                    continue;
                }

                int dxShiftIndex = y * mSamplesTextureWidth + fx;
                int dyShiftIndex = fy * mSamplesTextureWidth + x;

                XMVECTOR originPosition = XMVectorSet(static_cast<float>(x), _samples[baseIndex], static_cast<float>(y), 0.0f);
                XMVECTOR dxPosition = XMVectorSet(static_cast<float>(fx), _samples[dxShiftIndex], static_cast<float>(y), 0.0f);
                XMVECTOR dyPosition = XMVectorSet(static_cast<float>(x), _samples[dyShiftIndex], static_cast<float>(fy), 0.0f);

                XMVECTOR dx = XMVectorSubtract(dxPosition, originPosition);
                XMVECTOR dy = XMVectorSubtract(dyPosition, originPosition);
                sum = XMVectorAdd(sum, XMVector3Cross(dy, dx));
            }

            sum = XMVector3Normalize(sum);
            XMStoreFloat3(&_normals[baseIndex], sum);
        }
    }
}

std::vector<unsigned char> WaterSurface::buildNormalMap()
{
    auto convertNormalCoordToColor = [](float coord) -> unsigned char {
        coord = std::max(-1.0f, std::min(1.0f, coord));
        return static_cast<unsigned char>(255.0f * (coord + 1.0f) / 2.0f);
    };

    unsigned totalSamples = mSamplesTextureWidth * mSamplesTextureHeight;
    std::vector<unsigned char> normalMapData(3 * totalSamples);

    for (unsigned y = 0; y < mSamplesTextureHeight; ++y)
    {
        for (unsigned x = 0; x < mSamplesTextureWidth; ++x)
        {
            int baseIndex = y * mSamplesTextureWidth + x;
            XMFLOAT3 normal = _normals[baseIndex];

            normalMapData[3 * baseIndex + 0] = convertNormalCoordToColor(normal.x);
            normalMapData[3 * baseIndex + 1] = convertNormalCoordToColor(normal.y);
            normalMapData[3 * baseIndex + 2] = convertNormalCoordToColor(normal.z);
        }
    }

    return normalMapData;
}

void WaterSurface::copyNormalsToTexture(std::vector<unsigned char>& normals)
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

    if (mpNormalMapTexture != nullptr)
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource{};

        ID3D11Resource* resource = nullptr;
        mpNormalMapTexture->GetResource(&resource);
        HR(pDX11Renderer->getContext()->Map(resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

        memcpy((unsigned char*)mappedResource.pData, normals.data(), normals.size());

        pDX11Renderer->getContext()->Unmap(resource, 0);
    }
}

void GridSurface::init()
{
}
