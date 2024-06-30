module;

#include "mg.hpp"
#include "utils.h"

#include <d3d11.h>
#include <wrl/client.h>

export module core.renderables;
import std.core;
import dx11renderer;

export
{
    class Point : public IRenderable
    {
        inline static unsigned counter = 1;
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
            DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(pRenderer);

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
            cb.flags = 2;

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

        static std::vector<unsigned> produceTopology(int segments)
        {
            std::vector<unsigned> topology;

            for (int i = 0; i < segments; ++i)
            {
                for (int j = 0; j < segments; ++j)
                {
                    unsigned first = (i * (segments + 1)) + j;
                    unsigned second = first + segments + 1;

                    topology.insert(topology.end(), {first, second, first + 1});
                    topology.insert(topology.end(), {second, second + 1, first + 1});
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
        inline static unsigned counter = 1;

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
        IBezier(const std::unordered_set<IRenderable::Id>& selectedRenderableIds, std::string_view tag)
            : IRenderable{XMVectorZero(), tag}, mControlPointRenderableIds{selectedRenderableIds}
        {
        }

        static constexpr unsigned controlPointsNumber = 4;

        std::unordered_set<IRenderable::Id> mControlPointRenderableIds;
        bool mIsPolygon{};

        void insertControlPoint(const IRenderable::Id mId)
        {
            mControlPointRenderableIds.insert(mId);
            generateGeometry();
        }
    };

    class BezierC0 final : public IBezier
    {
        inline static unsigned counter = 1;

        const IRenderer* mpRenderer;

    public:
        BezierC0(const std::unordered_set<IRenderable::Id>& selectedRenderableIds, const IRenderer* pRenderer)
            : IBezier{selectedRenderableIds, std::format("BezierC0 {}", counter++).c_str()}, mpRenderer{pRenderer}
        {
        }

        void generateGeometry() override
        {
            mGeometry.clear();

            unsigned idx = 0;

            for (const auto mId : mControlPointRenderableIds)
            {
                if ((idx != 0) && (idx % controlPointsNumber == 0))
                {
                    const XMFLOAT3 previousPoint = mGeometry.at(mGeometry.size() - 1);
                    mGeometry.push_back(previousPoint);
                    idx++;
                }

                auto pRenderable = mpRenderer->getRenderable(mId);
                if (pRenderable)
                {
                    const XMVECTOR positionVec = pRenderable->mWorldPos;
                    XMFLOAT3 position;
                    XMStoreFloat3(&position, positionVec);
                    mGeometry.push_back(position);
                    idx++;
                }

            }

            if (mGeometry.size() % controlPointsNumber != 0)
            {
                const XMFLOAT3 previousPoint = mGeometry.at(mGeometry.size() - 1);

                while (mGeometry.size() % controlPointsNumber != 0)
                {
                    mGeometry.push_back(previousPoint);
                }
            }
        }
    };

    class BezierC2 final : public IBezier
    {
        inline static unsigned counter = 1;

        const IRenderer* mpRenderer;

    public:
        BezierC2(const std::unordered_set<IRenderable::Id>& selectedRenderableIds, const IRenderer* pRenderer)
            : IBezier{selectedRenderableIds, std::format("BezierC2 {}", counter++).c_str()}, mpRenderer{pRenderer}
        {
        }

        void generateGeometry() override
        {
            mGeometry.clear();

            unsigned idx = 0;

            for (const auto mId : mControlPointRenderableIds)
            {
                if ((idx != 0) && (idx % controlPointsNumber == 0))
                {
                    const XMFLOAT3 previousPoint = mGeometry.at(mGeometry.size() - 1);
                    mGeometry.push_back(previousPoint);
                    idx++;
                }

                auto pRenderable = mpRenderer->getRenderable(mId);
                if (pRenderable)
                {
                    const XMVECTOR positionVec = pRenderable->mWorldPos;
                    XMFLOAT3 position;
                    XMStoreFloat3(&position, positionVec);
                    mGeometry.push_back(position);
                    idx++;
                }
            }

            if (mGeometry.size() % controlPointsNumber != 0)
            {
                const XMFLOAT3 previousPoint = mGeometry.at(mGeometry.size() - 1);

                while (mGeometry.size() % controlPointsNumber != 0)
                {
                    mGeometry.push_back(previousPoint);
                }
            }
        }
    };
}