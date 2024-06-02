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
    WaterSurface(IRenderer* pRenderer, float planeSize, unsigned int textureSize);

    void init() override;
    void draw(GlobalCB& cb) override;

    void update(float dt) override;

private:
    class HeightMapTexture
    {
    public:
        HeightMapTexture();

        void SetValue(int x, int y, float value)
        {
            height_map_texture[x][y] = value;
        }

        int GetSize()
        {
            return mTextureSize;
        }

        float GetValue(int x, int y)
        {
            return height_map_texture[x][y];
        }

    private:
        static const unsigned int mTextureSize = 256;
        float** height_map_texture;
    };

    class WaveTexture
    {
    public:
    public:
        WaveTexture(IRenderer* pRenderer);

        void Update();
        void SetValue(int x, int y, XMFLOAT3 value);
        unsigned int GetSize()
        {
            return 256;
        }

        ComPtr<ID3D11ShaderResourceView> GetTexture();

    private:
        unsigned int texture_stride;
        unsigned int txture_size;

        std::array<BYTE, 256 * 256 * 4> texture_data;

        ComPtr<ID3D11Texture2D> texture;
        ComPtr<ID3D11ShaderResourceView> texture_resource_view;
        IRenderer* mpRenderer;
    };

    class AbsorptionTexture
    {
    public:
        AbsorptionTexture();

        int GetSize()
        {
            return mTextureSize;
        }

        float GetValue(int x, int y)
        {
            return absorption_texture[x][y];
        }

    private:
        static const unsigned int mTextureSize = 256;
        float** absorption_texture;
    };

    void CreateRipple(int x, int y)
    {
        mpCurrentHeightMap->SetValue(x, y, 2.25f);
    }

    void UpdateHeightMap();
    void UpdateWaveTextureMap();
    void GetRandomDrop()
    {
        static std::uniform_real_distribution<float> random(0, 100);

        if (random(mRandom) > 65.0f)
        {
            static std::uniform_real_distribution<float> random_x(0, 256);
            static std::uniform_real_distribution<float> random_y(0, 256);

            CreateRipple((int)random_x(mRandom), (int)random_y(mRandom));
        }
    }

    std::default_random_engine mRandom;

    WaveTexture mWaveTexture;
    AbsorptionTexture absorption_texture;
    std::array<HeightMapTexture, 3> height_map_textures;
    HeightMapTexture* mpNextHeightMap;
    HeightMapTexture* mpCurrentHeightMap;
    HeightMapTexture* mpPreviousHeightMap;

    ComPtr<ID3D11Buffer> mpVertexBuffer, mpIndexBuffer;
    ComPtr<ID3D11InputLayout> mpInputLayout{};
    ComPtr<ID3D11RasterizerState> mpRasterizer;

    unsigned int mTextureSize;
    float mPlaneWidth, mPlaneHeight;

    XMMATRIX mTexMtx;
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

WaterSurface::WaterSurface(IRenderer* pRenderer, float planeSize, unsigned int textureSize)
    : ISurface{pRenderer}, mPlaneWidth{planeSize}, mTextureSize{textureSize}, mWaveTexture{mpRenderer}
{
}

void WaterSurface::init()
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it

    createPlane(pDX11Renderer->getDevice(), mPlaneWidth, mPlaneHeight, mpVertexBuffer.GetAddressOf(),
                mpIndexBuffer.GetAddressOf());

    mpNextHeightMap = &this->height_map_textures[0];
    mpCurrentHeightMap = &this->height_map_textures[1];
    mpPreviousHeightMap = &this->height_map_textures[2];

    const D3D11_RASTERIZER_DESC desc{
        .FillMode = D3D11_FILL_SOLID,
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

    HR(pDX11Renderer->getDevice()->CreateRasterizerState(&desc, mpRasterizer.GetAddressOf()));
}


void WaterSurface::update(float dt)
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it

    UpdateHeightMap();
    UpdateWaveTextureMap();
    GetRandomDrop();

    pDX11Renderer->getContext()->PSSetShaderResources(1, 1, &mWaveTexture.GetTexture());
}

void WaterSurface::draw(GlobalCB& cb)
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it

    pDX11Renderer->getContext()->RSSetState(mpRasterizer.Get());

    //water_effect.SetCameraPosition(m_camPos);
    //water_effect.Begin(m_device.context());

    //m_cbWorldMtx->Update(m_device.context(), water_mesh_matrix);
    //water_mesh.Render(m_device.context());

    //    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer); // TODO: fix it

    //auto normalMapData = buildNormalMap();
    //copyNormalsToTexture(normalMapData);

    pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().pWaterSurfaceVS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().pWaterSurfacePS.first.Get(), nullptr, 0);

    cb.texMtx = mTexMtx;
    pDX11Renderer->updateCB(cb);

    //pDX11Renderer->getContext()->PSSetShaderResources(0, 1, mpNormalMapTexture.GetAddressOf());
    //pDX11Renderer->getContext()->PSSetShaderResources(1, 1, mpCubeMap.GetAddressOf());

    //ID3D11SamplerState* pSamplers[]{mpSamplerState.Get(), mpSamplerState.Get()};
    //pDX11Renderer->getContext()->PSSetSamplers(0, 2, pSamplers);

    pDX11Renderer->getContext()->IASetVertexBuffers(0, 1, mpVertexBuffer.GetAddressOf(), &stride, &offset);
    pDX11Renderer->getContext()->IASetIndexBuffer(mpIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pDX11Renderer->getContext()->DrawIndexed(6, 0, 0);
}

void WaterSurface::UpdateHeightMap()
{
    auto tmp_height_map_texture = mpPreviousHeightMap;
    mpPreviousHeightMap = mpCurrentHeightMap;
    mpCurrentHeightMap = mpNextHeightMap;
    mpNextHeightMap = tmp_height_map_texture;

    const float wave_speed = 1.0f;
    const float integration_step = 1.0f / mTextureSize;

    float h_value = 2.0f / (mTextureSize - 1);
    const float a_coef = wave_speed * wave_speed * integration_step * integration_step / (h_value * h_value);
    const float b_coef = 2.0f - 4.0f * a_coef;

    for (unsigned int y = 0; y < mTextureSize; ++y)
    {
        for (unsigned int x = 0; x < mTextureSize; ++x)
        {
            int lastX = x > 0 ? x - 1 : 0;
            int nextX = x < mTextureSize - 1 ? x + 1 : mTextureSize - 1;

            int lastY = y > 0 ? y - 1 : 0;
            int nextY = y < mTextureSize - 1 ? y + 1 : mTextureSize - 1;

            float absorption_value = absorption_texture.GetValue(x, y);
            float value =
                absorption_value *
                (a_coef *
                     (mpCurrentHeightMap->GetValue(nextX, y) + mpCurrentHeightMap->GetValue(lastX, y) +
                      mpCurrentHeightMap->GetValue(x, lastY) + mpCurrentHeightMap->GetValue(x, nextY)) +
                 b_coef * mpCurrentHeightMap->GetValue(x, y) - mpPreviousHeightMap->GetValue(x, y));

            mpNextHeightMap->SetValue(x, y, value);
        }
    }
}

void WaterSurface::UpdateWaveTextureMap()
{
    int mTextureSize = mWaveTexture.GetSize();

    for (int y = 0; y < mTextureSize; ++y)
    {
        for (int x = 0; x < mTextureSize; ++x)
        {
            int prevX = (x + mTextureSize - 1) % mTextureSize;
            int nextX = (x + 1) % mTextureSize;
            int prevY = (y + mTextureSize - 1) % mTextureSize;
            int nextY = (y + 1) % mTextureSize;

            auto p00 = mpNextHeightMap->GetValue(prevX, prevY);
            auto p10 = mpNextHeightMap->GetValue(x, prevY);
            auto p20 = mpNextHeightMap->GetValue(nextX, prevY);

            auto p01 = mpNextHeightMap->GetValue(prevX, y);
            auto p11 = mpNextHeightMap->GetValue(x, y);
            auto p21 = mpNextHeightMap->GetValue(nextX, y);

            auto p02 = mpNextHeightMap->GetValue(prevX, nextY);
            auto p12 = mpNextHeightMap->GetValue(x, nextY);
            auto p22 = mpNextHeightMap->GetValue(nextX, nextY);

            float horizontal = (p01 - p21) * 2.0f + p20 + p22 - p00 - p02;
            float vertical = (p12 - p10) * 2.0f + p22 + p02 - p20 - p00;
            float depth = 1.0f / 2.0f;

            float r = horizontal;
            float g = vertical;
            float b = depth;

            float length = std::sqrt(r * r + g * g + b * b);
            r /= length;
            g /= length;
            b /= length;

            mWaveTexture.SetValue(x, y, XMFLOAT3{r, g, b});
        }
    }
    mWaveTexture.Update();
}

WaterSurface::AbsorptionTexture::AbsorptionTexture()
{
    /* Allocate Mamory */
    absorption_texture = new float*[mTextureSize];

    for (int i = 0; i < mTextureSize; i++)
        absorption_texture[i] = new float[mTextureSize];

    /* Calculate Distance Data */
    float half_size_value = mTextureSize / 2.0f;

    for (int x = 0; x < mTextureSize; x++)
        for (int y = 0; y < mTextureSize; y++)
        {
            float posX = x / static_cast<float>(mTextureSize - 1);
            float posY = y / static_cast<float>(mTextureSize - 1);

            float left = posX;
            float right = 1.0f - posX;
            float top = posY;
            float bottom = 1.0f - posY;

            float leftDistance = std::max(left, std::max(top, bottom));
            float rightDistance = std::max(right, std::max(top, bottom));
            float topDistance = std::max(top, std::max(left, right));
            float bottomDistance = std::max(bottom, std::max(left, right));

            float l = std::min(std::min(leftDistance, rightDistance), std::min(topDistance, bottomDistance));
            float absorption_value = 0.95f * std::min(1.0f, l * 5.0f);

            absorption_texture[x][y] = absorption_value;
        }
}

WaterSurface::HeightMapTexture::HeightMapTexture()
{
    height_map_texture = new float*[mTextureSize];

    for (int i = 0; i < mTextureSize; i++)
        height_map_texture[i] = new float[mTextureSize];

    for (int x = 0; x < mTextureSize; x++)
        for (int y = 0; y < mTextureSize; y++)
            height_map_texture[x][y] = 0.0f;
}

WaterSurface::WaveTexture::WaveTexture(IRenderer* pRenderer) : mpRenderer(pRenderer)
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

    unsigned int texture_bpp = 4u;
    unsigned int texture_width = 256u;
    unsigned int texture_height = 256u;

    texture_stride = texture_width * texture_bpp;
    txture_size = texture_stride * texture_height;

    D3D11_TEXTURE2D_DESC desc{
        .Width = texture_width,
        .Height = texture_height,
        .MipLevels = 0,
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

    desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    pDX11Renderer->getDevice()->CreateTexture2D(&desc, 0, texture.GetAddressOf());
    pDX11Renderer->getDevice()->CreateShaderResourceView(texture.Get(), nullptr, texture_resource_view.GetAddressOf());

    BYTE* value_pointer = texture_data.data();
}

void WaterSurface::WaveTexture::SetValue(int x, int y, XMFLOAT3 value)
{
    BYTE* value_pointer = texture_data.data();

    value_pointer += (256 * 4 * y + 4 * x);

    *(value_pointer++) = static_cast<BYTE>(255 * (value.x + 1) / 2);
    *(value_pointer++) = static_cast<BYTE>(255 * (value.y + 1) / 2);
    *(value_pointer++) = static_cast<BYTE>(255 * (value.z + 1) / 2);
    *(value_pointer++) = static_cast<BYTE>(255);
}

void WaterSurface::WaveTexture::Update()
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

    pDX11Renderer->getContext()->UpdateSubresource(texture.Get(), 0, nullptr, texture_data.data(), texture_stride,
                                          txture_size);
}

ComPtr<ID3D11ShaderResourceView> WaterSurface::WaveTexture::GetTexture()
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(mpRenderer);

    ID3D11ShaderResourceView* pTexture;
    HR(pDX11Renderer->getDevice()->CreateShaderResourceView(texture.Get(), nullptr, &pTexture));

    return pTexture;
}