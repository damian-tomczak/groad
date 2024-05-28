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
    virtual void draw() = 0;

protected:
    ISurface(IRenderer* pRenderer) : mpRenderer{pRenderer}
    {

    }

    IRenderer* mpRenderer;
};

export class WaterSurface : public ISurface
{
public:
    WaterSurface(IRenderer* pRenderer, float planeWidth, float planeHeight, int samplesTextureWidth,
                 int samplesTextureHeight);

    void create();

    void init() override;
    void draw() override;

    void applyDisturbanceInWorldSpace(XMFLOAT3 position, float strength);
    void update(float deltaTime) override;

    void setCubemap(ID3D11ShaderResourceView* cubemap)
    {
        _cubemap = cubemap;
    }
    ID3D11ShaderResourceView* getCubemap()
    {
        return _cubemap;
    }

    XMMATRIX getModelMatrix()
    {
        return _modelMatrix;
    }
    void setModelMatrix(XMMATRIX modelMatrix);

protected:
    void clearSamples();
    void calculateNormalMap();
    std::vector<unsigned char> buildNormalMap();
    void copyNormalsToTexture(std::vector<unsigned char>& normals);

    unsigned char convertNormalCoordToColor(float coord);

private:
    ComPtr<ID3D11Buffer> mpVertexBuffer, mpIndexBuffer;
    ID3D11ShaderResourceView* _cubemap;

    float mPlaneWidth, mPlaneHeight;
    unsigned mSamplesTextureWidth, mSamplesTextureHeight;
    std::vector<float> _samples, _samples2;
    std::vector<XMFLOAT3> _normals;
    ID3D11ShaderResourceView* _normalMapTexture;

    std::vector<float>* _currentSamples, *_previousSamples;

    XMMATRIX _modelMatrix, _invModelMatrix, _textureMatrix;

    ID3D11VertexShader* _vertexShader;
    ID3D11PixelShader* _pixelShader;
    ID3D11InputLayout* _inputLayout;
};

export class GridSurface : public ISurface
{
public:
    GridSurface(IRenderer* pRenderer) : ISurface{pRenderer}
    {

    }

    void init() override;
    void draw() override
    {
        DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it
        ID3D11DeviceContext* const pContext = pDX11Renderer->getContext();

        pContext->VSSetShader(pDX11Renderer->mpGridVS.Get(), nullptr, 0);
        pContext->PSSetShader(pDX11Renderer->mpGridPS.Get(), nullptr, 0);
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pContext->Draw(6, 0);
    }

    void update(float dt) override
    {

    }
};

module :private;

void createPlane(ID3D11Device* pDevice, float width, float length, ComPtr<ID3D11Buffer> pVertexBuffer, ComPtr<ID3D11Buffer> pIndexBuffer)
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

    HR(pDevice->CreateBuffer(&bufferDesc, &initData, &pVertexBuffer));

    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(indices);
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;
    initData.pSysMem = indices;

    HR(pDevice->CreateBuffer(&bufferDesc, &initData, &pIndexBuffer));
}

WaterSurface::WaterSurface(IRenderer* pRenderer, float planeWidth, float planeHeight, int samplesTextureWidth, int samplesTextureHeight)
    : ISurface{pRenderer}, _cubemap(nullptr), _normalMapTexture(nullptr),
      _vertexShader(nullptr), _pixelShader(nullptr), _inputLayout(nullptr), mPlaneWidth(planeWidth), mPlaneHeight(planeHeight),
      mSamplesTextureWidth(samplesTextureWidth), mSamplesTextureHeight(samplesTextureHeight)
{
    _modelMatrix = XMMatrixIdentity();
    _invModelMatrix = XMMatrixInverse(nullptr, _modelMatrix);
}

void WaterSurface::applyDisturbanceInWorldSpace(XMFLOAT3 position, float strength)
{
    // Convert position to XMVECTOR for matrix operations
    XMVECTOR posVec = XMLoadFloat3(&position);
    posVec = XMVectorSetW(posVec, 1.0f); // Set W component to 1 for transformation

    // Calculate texture space position using DirectX Math
    XMMATRIX invModelMatrix = XMMatrixInverse(nullptr, _modelMatrix);
    XMVECTOR textureSpacePositionVec = XMVector3TransformCoord(posVec, invModelMatrix);
    textureSpacePositionVec = XMVector3TransformCoord(textureSpacePositionVec, _textureMatrix);

    // Extract X and Z components for texture coordinate calculation
    XMFLOAT3 textureSpacePosition;
    XMStoreFloat3(&textureSpacePosition, textureSpacePositionVec);

    // Check if the disturbance is within the bounds of the texture
    if (textureSpacePosition.x < 0.0f || textureSpacePosition.x > 1.0f || textureSpacePosition.z < 0.0f ||
        textureSpacePosition.z > 1.0f)
    {
        return;
    }

    // Calculate texture coordinates
    int coordX = static_cast<int>(textureSpacePosition.x * mSamplesTextureWidth);
    int coordY = static_cast<int>(textureSpacePosition.z * mSamplesTextureHeight);

    // Apply the disturbance strength to the appropriate sample
    (*_currentSamples)[coordY * mSamplesTextureWidth + coordX] += strength;
}

void WaterSurface::update(float deltaTime)
{
    int N = 256; // Usually, N should be defined based on your texture size or passed as a parameter
    float h = 2.0f / (N - 1);
    float c = 1.0f;      // Speed of wave propagation
    float dt = 1.0f / N; // Time step, might be adjusted based on real-time requirements

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
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags = 0,
        .MiscFlags = 0,
    };

    auto normalMapData = buildNormalMap();

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = &normalMapData[0];
    initData.SysMemPitch = mSamplesTextureWidth * sizeof(unsigned char) * 4;

    ID3D11Texture2D* pTexture{};
    HR(pDX11Renderer->getDevice()->CreateTexture2D(&texDesc, &initData, &pTexture));

    pDX11Renderer->getDevice()->CreateShaderResourceView(pTexture, nullptr, &_normalMapTexture);
    pTexture->Release();

    createPlane(pDX11Renderer->getDevice(), mPlaneWidth, mPlaneHeight, mpVertexBuffer, mpIndexBuffer);

    _textureMatrix = XMMatrixScaling(1.0f / mPlaneWidth, 0, 1.0f / mPlaneHeight);
    _textureMatrix = XMMatrixTranslation(0.5f * mPlaneWidth, 0, 0.5f * mPlaneHeight) * _textureMatrix;
}

void WaterSurface::draw()
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it

    auto normalMapData = buildNormalMap();
    copyNormalsToTexture(normalMapData);

    // Set the shaders
    pDX11Renderer->getContext()->VSSetShader(_vertexShader, nullptr, 0);
    pDX11Renderer->getContext()->PSSetShader(_pixelShader, nullptr, 0);

    // Set the texture and cube map
    pDX11Renderer->getContext()->PSSetShaderResources(0, 1, &_normalMapTexture);
    pDX11Renderer->getContext()->PSSetShaderResources(1, 1, &_cubemap);

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(ConstantBufferData);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Buffer* pConstantBuffer = nullptr;
    pDX11Renderer->getDevice()->CreateBuffer(&bd, nullptr, &pConstantBuffer);

    // Set the constant buffer in the vertex shader and pixel shader
    pDX11Renderer->getContext()->VSSetConstantBuffers(0, 1, &pConstantBuffer);
    pDX11Renderer->getContext()->PSSetConstantBuffers(0, 1, &pConstantBuffer);

    // Bind the vertex array object and draw
    UINT stride = sizeof(XMFLOAT3);
    UINT offset = 0;
    pDX11Renderer->getContext()->IASetVertexBuffers(0, 1, &mpVertexBuffer, &stride, &offset);
    pDX11Renderer->getContext()->IASetIndexBuffer(mpIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pDX11Renderer->getContext()->DrawIndexed(6, 0, 0); // Assumes you're drawing two triangles (a quad)

    // Release resources
    pConstantBuffer->Release();
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
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it
    // Assuming _normalMapTexture is an ID3D11Texture2D* and already created

    // Update the texture with the normal data
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

    ID3D11Resource* resource = nullptr;
    _normalMapTexture->GetResource(&resource);
    HRESULT hr = pDX11Renderer->getContext()->Map(resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr))
    {
        // Copy the normal data to the texture
        for (UINT row = 0; row < mSamplesTextureHeight; ++row)
        {
            memcpy((unsigned char*)mappedResource.pData + row * mappedResource.RowPitch,
                   normals.data() + row * mSamplesTextureWidth * 3, // Assuming 3 bytes per pixel (RGB)
                   mSamplesTextureWidth * 3);
        }
        // Unmap the texture to apply changes
        pDX11Renderer->getContext()->Unmap(resource, 0);
    }
    else
    {
        // Handle errors appropriately
        std::cerr << "Failed to map texture resource for updating." << std::endl;
    }
}

void GridSurface::init()
{
}
