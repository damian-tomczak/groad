module;

#include "mg.hpp"
#include "utils.h"

#include <d3d11.h>
#include <wrl/client.h>

#define COUNTER()                       \
    friend void reloadCounters();       \
    inline static unsigned counter = 1; \

export module core.renderables;
import dx11renderer;

export
{
    class CounterReloader;

    class Point : public IRenderable
    {
        COUNTER()

        static constexpr float defaultRadius = 0.1f;
        static constexpr int defaultNumberOfSegments = 50;

    public:
        Point(XMVECTOR pos, float radius = defaultRadius, int segments = defaultNumberOfSegments)
            : IRenderable{pos, std::format("Point {}", counter++).c_str()}, mRadius(radius), mSegments(segments)
        {
        }

        float mRadius;
        int mSegments;

        static void drawPrimitive(IRenderer* pRenderer, GlobalCB& cb, XMVECTOR pos, float radius = defaultRadius,
                                  int segments = defaultNumberOfSegments)
        {
            auto pDX11Renderer = static_cast<DX11Renderer*>(pRenderer);

            std::vector<XMFLOAT3> geometry = produceGeometry(radius, segments);
            std::vector<unsigned> topology = produceTopology(segments);

            ComPtr<ID3D11Buffer> pVertexBuffer;
            ComPtr<ID3D11Buffer> pIndexBuffer;

            D3D11_BUFFER_DESC vbd{
                .ByteWidth = static_cast<UINT>(sizeof(XMFLOAT3) * geometry.size()),
                .Usage = D3D11_USAGE_DYNAMIC,
                .BindFlags = D3D11_BIND_VERTEX_BUFFER,
                .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
                .MiscFlags = 0,
                .StructureByteStride = 0,
            };
            D3D11_SUBRESOURCE_DATA vinitData{
                .pSysMem = geometry.data(),
            };
            HR(pDX11Renderer->getDevice()->CreateBuffer(&vbd, &vinitData, &pVertexBuffer));

            D3D11_BUFFER_DESC ibd{
                .ByteWidth = static_cast<UINT>(sizeof(unsigned) * topology.size()),
                .Usage = D3D11_USAGE_DYNAMIC,
                .BindFlags = D3D11_BIND_INDEX_BUFFER,
                .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
                .MiscFlags = 0,
                .StructureByteStride = 0,
            };
            D3D11_SUBRESOURCE_DATA initData{
                .pSysMem = topology.data(),
            };
            HR(pDX11Renderer->getDevice()->CreateBuffer(&ibd, &initData, &pIndexBuffer));

            const XMMATRIX translationMat = XMMatrixTranslationFromVector(pos);
            cb.modelMtx = translationMat;

            cb.color = Color{0.5f, 0.5f, 0.5f, 1.0f};
            pDX11Renderer->updateCB(cb);

            UINT vStride = sizeof(XMFLOAT3), offset = 0;
            pDX11Renderer->getContext()->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &vStride,
                                                            &offset);
            pDX11Renderer->getContext()->IASetIndexBuffer(pIndexBuffer.Get(),
                                                          DXGI_FORMAT_R32_UINT, 0);

            pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().defaultVS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().defaultPS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->HSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->DSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            pDX11Renderer->getContext()->DrawIndexed(static_cast<UINT>(topology.size()), 0, 0);
        }

        static std::vector<XMFLOAT3> produceGeometry(float radius, int segments)
        {
            std::vector<XMFLOAT3> geometry;

            for (int i = 0; i <= segments; ++i)
            {
                float theta = (i / static_cast<float>(segments)) * std::numbers::pi_v<float>;
                for (int j = 0; j <= segments; ++j)
                {
                    float phi = (j / static_cast<float>(segments)) * 2 * std::numbers::pi_v<float>;
                    float x = radius * std::sin(theta) * std::cos(phi);
                    float y = radius * std::sin(theta) * std::sin(phi);
                    float z = radius * std::cos(theta);
                    geometry.emplace_back(x, y, z);
                }
            }

            return geometry;
        }

        static std::vector<unsigned int> produceTopology(int segments)
        {
            std::vector<unsigned int> topology;

            for (int i = 0; i < segments; ++i)
            {
                for (int j = 0; j < segments; ++j)
                {
                    const unsigned int topLeft = i * (segments + 1) + j;
                    unsigned int topRight = topLeft + 1;
                    const unsigned int bottomLeft = topLeft + (segments + 1);
                    unsigned int bottomRight = bottomLeft + 1;

                    if (j == segments - 1)
                    {
                        topRight = i * (segments + 1);
                        bottomRight = (i + 1) * (segments + 1);
                    }

                    topology.insert(topology.end(), {topLeft, bottomLeft, topRight});
                    topology.insert(topology.end(), {topRight, bottomLeft, bottomRight});
                }
            }

            return topology;
        }

        void generateGeometry() override
        {
            mGeometry = produceGeometry(mRadius, mSegments);
        }

        void generateTopology() override
        {
            mTopology = produceTopology(mSegments);
        }
    };

    class Torus : public IRenderable
    {
        COUNTER();

    public:
        Torus(XMVECTOR pos, float majorRadius = 0.7f, float minorRadius = 0.2f, int majorSegments = 100,
              int minorSegments = 20)
            : IRenderable{pos, std::format("Torus {}", counter++).c_str()}, mMajorRadius(majorRadius),
              mMinorRadius(minorRadius), mMajorSegments(majorSegments), mMinorSegments(minorSegments)
        {
        }

        float mMajorRadius;
        float mMinorRadius;
        int mMajorSegments;
        int mMinorSegments;

        void generateGeometry() override
        {
            mGeometry.clear();
            for (int i = 0; i <= mMajorSegments; ++i)
            {
                float theta = (i / static_cast<float>(mMajorSegments)) * 2 * std::numbers::pi_v<float>;
                for (int j = 0; j <= mMinorSegments; ++j)
                {
                    float phi = (j / static_cast<float>(mMinorSegments)) * 2 * std::numbers::pi_v<float>;
                    float x = (mMajorRadius + mMinorRadius * cos(phi)) * cos(theta);
                    float y = (mMajorRadius + mMinorRadius * cos(phi)) * sin(theta);
                    float z = mMinorRadius * sin(phi);
                    mGeometry.emplace_back(x, y, z);
                }
            }
        }

        void generateTopology() override
        {
            mTopology.clear();
            for (int i = 0; i < mMajorSegments; ++i)
            {
                for (int j = 0; j < mMinorSegments; ++j)
                {
                    unsigned first = (i * (mMinorSegments + 1)) + j;
                    unsigned nextInMinor = first + 1;
                    unsigned nextInMajor = (first + mMinorSegments + 1) % (mMajorSegments * (mMinorSegments + 1));

                    mTopology.insert(mTopology.end(), {first, nextInMinor});
                    mTopology.insert(mTopology.end(), {first, nextInMajor});
                }
            }
        }
    };

    class IBezier : public IRenderable
    {
    public:
        IBezier(const std::vector<Id>& selectedRenderableIds, std::string_view tag, Color color)
            : IRenderable{XMVectorZero(), tag, color}, mDeBoorIds{selectedRenderableIds}
        {
        }

        static constexpr unsigned deBoorNumber = 4;

        std::vector<Id> mDeBoorIds;
        bool mIsPolygon{};

        void insertDeBoorPointId(const Id id)
        {
            mDeBoorIds.push_back(id);
            generateGeometry();
        }

        bool isScalable() override
        {
            return true;
        }
    };

    class BezierC0 final : public IBezier
    {
        COUNTER();

        const IRenderer* mpRenderer;

    public:
        BezierC0(const std::vector<Id>& selectedRenderableIds, const IRenderer* pRenderer, Color color = Colors::defaultColor)
            : IBezier{selectedRenderableIds, std::format("BezierC0 {}", counter++).c_str(), color}, mpRenderer{pRenderer}
        {
        }

        void generateGeometry() override
        {
            mGeometry.clear();

            unsigned idx = 0;

            for (const auto id : mDeBoorIds)
            {
                if ((idx != 0) && (idx % deBoorNumber == 0))
                {
                    const XMFLOAT3 previousPoint = mGeometry.at(mGeometry.size() - 1);
                    mGeometry.push_back(previousPoint);
                    idx++;
                }

                auto pRenderable = mpRenderer->getRenderable(id);
                if (pRenderable)
                {
                    const XMVECTOR positionVec = pRenderable->mWorldPos;
                    XMFLOAT3 position;
                    XMStoreFloat3(&position, positionVec);
                    mGeometry.push_back(position);
                    idx++;
                }
            }

            if (mGeometry.size() % deBoorNumber != 0)
            {
                const XMFLOAT3 previousPoint = mGeometry.at(mGeometry.size() - 1);

                while (mGeometry.size() % deBoorNumber != 0)
                {
                    mGeometry.push_back(previousPoint);
                }
            }
        }
    };

    class BezierC2 final : public IBezier
    {
        COUNTER();

        const IRenderer* mpRenderer;

    public:
        BezierC2(const std::vector<Id>& selectedRenderableIds, const IRenderer* pRenderer, Color color = Colors::defaultColor)
            : IBezier{selectedRenderableIds, std::format("BezierC2 {}", counter++).c_str(), color}, mpRenderer{pRenderer}
        {
        }

        void regenerateData()
        {
            IRenderable::regenerateData();

            generateBernsteinPoints();
        }

        void generateGeometry() override
        {
            mGeometry.clear();

            unsigned idx = 0;

            for (const auto id : mDeBoorIds)
            {
                if ((idx != 0) && (idx % deBoorNumber == 0))
                {
                    const XMFLOAT3& previousPoint3 = mGeometry.at(mGeometry.size() - 3);
                    const XMFLOAT3& previousPoint2 = mGeometry.at(mGeometry.size() - 2);
                    const XMFLOAT3& previousPoint1 = mGeometry.at(mGeometry.size() - 1);
                    mGeometry.insert(mGeometry.end(), {previousPoint3, previousPoint2, previousPoint1});
                    idx += 3;
                }

                auto pRenderable = mpRenderer->getRenderable(id);
                if (pRenderable)
                {
                    const XMVECTOR positionVec = pRenderable->getGlobalPos();
                    XMFLOAT3 position;
                    XMStoreFloat3(&position, positionVec);
                    mGeometry.push_back(position);
                    idx++;
                }
            }

            if (mGeometry.size() % deBoorNumber != 0)
            {
                const XMFLOAT3 previousPoint = mGeometry.at(mGeometry.size() - 1);

                while (mGeometry.size() % deBoorNumber != 0)
                {
                    mGeometry.push_back(previousPoint);
                }
            }
        }

        std::vector<XMFLOAT3>& getBernsteinPositions()
        {
            return mGeometry;
        }

    private:
        void generateBernsteinPoints()
        {
            static const XMMATRIX bSplineToBernstein
            {
                1.0f / 6, 2.0f / 3, 1.0f / 6, 0.0f,
                0.0f,     2.0f / 3, 1.0f / 3, 0.0f,
                0.0f,     1.0f / 3, 2.0f / 3, 0.0f,
                0.0f,     1.0f / 6, 2.0f / 3, 1.0f / 6
            };

            for (size_t i = 0; i < mGeometry.size(); i += 4)
            {
                XMVECTOR pos1 = XMLoadFloat3(&mGeometry[i]);
                XMVECTOR pos2 = XMLoadFloat3(&mGeometry[i + 1]);
                XMVECTOR pos3 = XMLoadFloat3(&mGeometry[i + 2]);
                XMVECTOR pos4 = XMLoadFloat3(&mGeometry[i + 3]);

                XMMATRIX pointsMat = XMMATRIX(pos1, pos2, pos3, pos4);

                XMMATRIX pointsInBspline = XMMatrixMultiply(bSplineToBernstein, pointsMat);

                XMStoreFloat3(&mGeometry[i],     pointsInBspline.r[0]);
                XMStoreFloat3(&mGeometry[i + 1], pointsInBspline.r[1]);
                XMStoreFloat3(&mGeometry[i + 2], pointsInBspline.r[2]);
                XMStoreFloat3(&mGeometry[i + 3], pointsInBspline.r[3]);
            }
        }
    };

    inline void reloadCounters()
    {
        Point::counter = Torus::counter = BezierC0::counter = BezierC2::counter = 1;
    }
}