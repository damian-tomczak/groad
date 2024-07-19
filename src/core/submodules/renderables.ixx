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
            : IRenderable{std::format("Point {}", counter++).c_str(), pos}, mRadius(radius), mSegments(segments)
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

            //cb.color = Color{0.5f, 0.5f, 0.5f, 1.0f};
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
            IRenderable::generateGeometry();

            mGeometry = produceGeometry(mRadius, mSegments);
        }

        void generateTopology() override
        {
            IRenderable::generateTopology();

            mTopology = produceTopology(mSegments);
        }
    };

    class Torus : public IRenderable
    {
        COUNTER();

    public:
        Torus(XMVECTOR pos, float majorRadius = 0.7f, float minorRadius = 0.2f, int majorSegments = 100,
              int minorSegments = 20)
            : IRenderable{std::format("Torus {}", counter++).c_str(), pos}, mMajorRadius(majorRadius),
              mMinorRadius(minorRadius), mMajorSegments(majorSegments), mMinorSegments(minorSegments)
        {
        }

        float mMajorRadius;
        float mMinorRadius;
        int mMajorSegments;
        int mMinorSegments;

        void generateGeometry() override
        {
            IRenderable::generateGeometry();

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
            IRenderable::generateTopology();

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
            : IRenderable{tag, XMVectorZero(), color}, mDeBoorIds{selectedRenderableIds}
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
        BezierC0(const std::vector<Id>& selectedRenderableIds, const IRenderer* pRenderer, Color color = defaultColor)
            : IBezier{selectedRenderableIds, std::format("BezierC0 {}", counter++).c_str(), color}, mpRenderer{pRenderer}
        {
        }

        void generateGeometry() override
        {
            IRenderable::generateGeometry();

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
        BezierC2(const std::vector<Id>& selectedRenderableIds, const IRenderer* pRenderer, Color color = defaultColor)
            : IBezier{selectedRenderableIds, std::format("BezierC2 {}", counter++).c_str(), color}, mpRenderer{pRenderer}
        {
        }

        void regenerateData() override
        {
            IRenderable::regenerateData();

            generateBernsteinPoints();
        }

        void generateGeometry() override
        {
            IRenderable::generateGeometry();

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

    class InterpolatedBezierC2 final : public IBezier
    {
        COUNTER();

        const IRenderer* mpRenderer;

    public:
        InterpolatedBezierC2(const std::vector<Id>& selectedRenderableIds, const IRenderer* pRenderer, Color color = defaultColor)
            : IBezier{selectedRenderableIds, std::format("InterpolatedBezierC2 {}", counter++).c_str(), color},
              mpRenderer{pRenderer}
        {
        }

        void generateGeometry() override
        {
            IRenderable::generateGeometry();

            std::vector<XMVECTOR> deBoorPositions;
            deBoorPositions.reserve(mDeBoorIds.size());

            if (mDeBoorIds.size() == 0)
            {
                return;
            }

            auto pFirstRenderable = mpRenderer->getRenderable(*mDeBoorIds.begin());
            if (pFirstRenderable != nullptr)
            {
                deBoorPositions.push_back(pFirstRenderable->getGlobalPos());
            }

            for (auto it = mDeBoorIds.begin() + 1; it != mDeBoorIds.end();)
            {
                if (*(it - 1) == *it)
                {
                    it = mDeBoorIds.erase(it);
                }
                else
                {
                    auto pRenderable = mpRenderer->getRenderable(*it);
                    if (pRenderable != nullptr)
                    {
                        const XMVECTOR posVec = pRenderable->getGlobalPos();
                        deBoorPositions.push_back(posVec);
                    }

                    ++it;
                }
            }

            auto [alfa, beta, b] = createSystemOfEquations(deBoorPositions);
            std::vector<XMVECTOR> cs = solveSplineSystem(alfa, beta, b);

            cs.insert(cs.begin(), XMVectorZero());
            cs.push_back(XMVectorZero());

            for (const XMVECTOR& posVec : getBezierPoints(cs, deBoorPositions))
            {
                XMFLOAT3 pos;
                XMStoreFloat3(&pos, posVec);
                mGeometry.push_back(pos);
            }
        }

    private:
        std::tuple<std::vector<float>, std::vector<float>, std::vector<XMVECTOR>> createSystemOfEquations(
            const std::vector<XMVECTOR>& points)
        {
            std::vector<float> alfa(points.size() - 2);
            std::vector<float> beta(points.size() - 2);
            std::vector<XMVECTOR> b(points.size() - 2);
            std::vector<float> chord(points.size() - 1);

            for (size_t i = 0; i < points.size() - 1; ++i)
            {
                XMVECTOR diff = XMVectorSubtract(points[i + 1], points[i]);
                XMVECTOR length = XMVector3Length(diff);
                chord[i] = XMVectorGetX(length);
            }

            for (size_t i = 1; i < points.size() - 1; ++i)
            {
                float chordSum = chord[i - 1] + chord[i];
                alfa[i - 1] = chord[i - 1] / chordSum;
                beta[i - 1] = chord[i] / chordSum;

                XMVECTOR diff1 =
                    XMVectorScale(XMVectorSubtract(points[i + 1], points[i]), 3.0f / chord[i]);
                XMVECTOR diff2 =
                    XMVectorScale(XMVectorSubtract(points[i], points[i - 1]), 3.0f / chord[i - 1]);
                b[i - 1] = XMVectorDivide(XMVectorSubtract(diff1, diff2),
                                                   XMVectorReplicate(chordSum));
            }

            return {alfa, beta, b};
        }

        std::vector<XMVECTOR> solveSplineSystem(const std::vector<float>& alfa, const std::vector<float>& beta,
                                                std::vector<XMVECTOR>& R)
        {
            if (alfa.size() == 1)
            {
                R[0] = XMVectorScale(R[0], 0.5f);
                return R;
            }

            if (alfa.size() != beta.size())
            {
                return {};
            }

            const size_t n = alfa.size() - 1;

            float lastBeta = beta[0] / 2.0f;
            R[0] = XMVectorScale(R[0], 0.5f);

            for (size_t i = 1; i < n; i++)
            {
                float denominator = 2.0f - alfa[i] * lastBeta;
                lastBeta = beta[i] / denominator;
                R[i] = XMVectorScale(XMVectorSubtract(R[i], XMVectorScale(R[i - 1], alfa[i])), 1.0f / denominator);
            }

            R[n] = XMVectorScale(XMVectorSubtract(R[n], XMVectorScale(R[n - 1], alfa[n])),
                                 1.0f / (2.0f - alfa[n] * lastBeta));

            for (size_t i = n; i-- > 0;)
            {
                R[i] = XMVectorSubtract(R[i], XMVectorScale(R[i + 1], lastBeta));

                if (i > 0)
                {
                    lastBeta = beta[i - 1] / (2.0f - alfa[i] * lastBeta);
                }
            }

            return R;
        }

        std::vector<XMVECTOR> getBezierPoints(const std::vector<XMVECTOR>& cs, const std::vector<XMVECTOR>& points)
        {
            const size_t n = points.size() - 1;

            std::vector<XMVECTOR> a(points.size());
            std::vector<XMVECTOR> b(points.size());
            std::vector<XMVECTOR> c(points.size());
            std::vector<XMVECTOR> d(points.size());

            std::vector<float> chord;

            for (size_t i = 0; i < n; ++i)
            {
                chord.push_back(XMVectorGetX(XMVector3Length(XMVectorSubtract(points[i + 1], points[i]))));
            }

            a[0] = points[0];
            c[0] = cs[0];

            for (size_t i = 1; i <= n; ++i)
            {
                a[i] = points[i];
                c[i] = cs[i];

                auto cDiff = XMVectorSubtract(c[i], c[i - 1]);
                auto cScaledDivided = XMVectorDivide(cDiff, XMVectorReplicate(6 * chord[i - 1]));
                d[i - 1] = XMVectorScale(cScaledDivided, 2);

                auto aDiff = XMVectorSubtract(a[i], a[i - 1]);
                auto cPrevScaled = XMVectorScale(c[i - 1], chord[i - 1] * chord[i - 1]);
                auto dPrevScaled = XMVectorScale(d[i - 1], chord[i - 1] * chord[i - 1] * chord[i - 1]);
                auto numerator = XMVectorSubtract(XMVectorSubtract(aDiff, cPrevScaled), dPrevScaled);
                b[i - 1] = XMVectorDivide(numerator, XMVectorReplicate(chord[i - 1]));
            }

            for (size_t i = 0; i < n; ++i)
            {
                b[i] = XMVectorScale(b[i], chord[i]);
                c[i] = XMVectorScale(c[i], chord[i] * chord[i]);
                d[i] = XMVectorScale(d[i], chord[i] * chord[i] * chord[i]);
            }

            // clang-format off
            static const XMMATRIX powerToBernstein = XMMatrixTranspose({
                1.0f, 0.0f,        0.0f,        0.0f,
                1.0f, 1.0f / 3.0f, 0.0f,        0.0f,
                1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 0.0f,
                1.0f, 1.0f,        1.0f,        1.0f,
            });
            // clang-format on

            std::vector<XMVECTOR> bezierPoints;

            for (size_t i = 0; i < n; ++i)
            {
                XMVECTOR xs = XMVector4Transform(
                    XMVectorSet(XMVectorGetX(a[i]), XMVectorGetX(b[i]), XMVectorGetX(c[i]), XMVectorGetX(d[i])),
                    powerToBernstein);
                XMVECTOR ys = XMVector4Transform(
                    XMVectorSet(XMVectorGetY(a[i]), XMVectorGetY(b[i]), XMVectorGetY(c[i]), XMVectorGetY(d[i])),
                    powerToBernstein);
                XMVECTOR zs = XMVector4Transform(
                    XMVectorSet(XMVectorGetZ(a[i]), XMVectorGetZ(b[i]), XMVectorGetZ(c[i]), XMVectorGetZ(d[i])),
                    powerToBernstein);

                bezierPoints.push_back(XMVectorSet(XMVectorGetX(xs), XMVectorGetX(ys), XMVectorGetX(zs), 1.0f));
                bezierPoints.push_back(XMVectorSet(XMVectorGetY(xs), XMVectorGetY(ys), XMVectorGetY(zs), 1.0f));
                bezierPoints.push_back(XMVectorSet(XMVectorGetZ(xs), XMVectorGetZ(ys), XMVectorGetZ(zs), 1.0f));

                if (i == n - 1)
                {
                    bezierPoints.push_back(points[n]);
                }
                else
                {
                    bezierPoints.push_back(XMVectorSet(XMVectorGetW(xs), XMVectorGetW(ys), XMVectorGetW(zs), 1.0f));
                }
            }

            return bezierPoints;
        }
    };

    struct BezierPatchCreator final
    {
        int patchCountWidth = 3;
        int patchCountLength = 5;
        float planeWidth = 1.0f;
        float planeLength = 1.0f;
        bool isWrapped = false;
        bool isC2 = false;
    };

    class BezierPatchC0 final : public IRenderable
    {
        COUNTER();

    public:
        BezierPatchC0(const BezierPatchCreator& bezierPatchCreator, Color color = defaultColor)
            : IRenderable{std::format("BezierPatchC0 {}", counter++).c_str(), color}
        {
        }

        void regenerateData() override
        {
            IRenderable::regenerateData();
        }

        void generateGeometry() override
        {
            IRenderable::generateGeometry();
        }
    };

    inline void reloadCounters()
    {
        Point::counter = Torus::counter = BezierC0::counter = BezierC2::counter = InterpolatedBezierC2::counter = 1;
    }
}