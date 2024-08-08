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
        Point(FXMVECTOR pos, float radius = defaultRadius, int segments = defaultNumberOfSegments)
            : IRenderable{"Point " + std::to_string(counter++), pos}, mRadius(radius), mSegments(segments)
        {
        }

        const unsigned int& getStride() const override
        {
            static constexpr unsigned int stride = sizeof(XMFLOAT3);
            return stride;
        }

        void draw(class IRenderer* pRenderer, unsigned long long int renderableIdx) override
        {
            DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(pRenderer);

            pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            pDX11Renderer->getContext()->IASetInputLayout(pDX11Renderer->getDefaultInputLayout());
            pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().defaultVS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->HSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->DSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().defaultPS.first.Get(), nullptr, 0);

            pDX11Renderer->getContext()->DrawIndexed(static_cast<UINT>(getTopology().size()), 0, 0);
        }

        static void drawPrimitive(IRenderer* pRenderer, XMVECTOR pos, float radius = defaultRadius,
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
            pDX11Renderer->mGlobalCB.modelMtx = translationMat;

            pDX11Renderer->mGlobalCB.color = Color{0.5f, 0.5f, 0.5f, 1.0f};
            pDX11Renderer->updateCB(pDX11Renderer->mGlobalCB);

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

        float mRadius;
        int mSegments;
    };

    class Torus : public IRenderable
    {
        COUNTER();

    public:
        Torus(FXMVECTOR pos, float majorRadius = 0.7f, float minorRadius = 0.2f, int majorSegments = 100,
              int minorSegments = 20)
            : IRenderable{"Torus " + std::to_string(counter++), pos}, mMajorRadius(majorRadius),
              mMinorRadius(minorRadius), mMajorSegments(majorSegments), mMinorSegments(minorSegments)
        {
        }

        const unsigned int& getStride() const override
        {
            static constexpr unsigned int stride = sizeof(XMFLOAT3);
            return stride;
        }

        void draw(class IRenderer* pRenderer, unsigned long long int renderableIdx) override
        {
            DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(pRenderer);

            pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

            pDX11Renderer->getContext()->IASetInputLayout(pDX11Renderer->getDefaultInputLayout());
            pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().defaultVS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->HSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->DSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().defaultPS.first.Get(), nullptr, 0);
        }

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

        float mMajorRadius;
        float mMinorRadius;
        int mMajorSegments;
        int mMinorSegments;
    };

    class IControlPointBased
    {
    protected:
        IControlPointBased(const std::vector<Id>& controlPointIds = std::vector<Id>())
            : mControlPointIds{controlPointIds}
        {

        };

    public:
        std::vector<Id> mControlPointIds;
    };

    class IBezier : public IRenderable, public IControlPointBased
    {
    public:
        IBezier(const std::vector<Id>& selectedRenderableIds, std::string&& tag, Color color)
            : IRenderable{std::move(tag)}, IControlPointBased{selectedRenderableIds}
        {
        }

        static constexpr unsigned deBoorNumber = 4;

        const unsigned int& getStride() const override
        {
            static constexpr unsigned int stride = 4 * sizeof(XMFLOAT3);
            return stride;
        }

        void draw(class IRenderer* pRenderer, unsigned long long int renderableIdx) override
        {
            auto pDX11Renderer = static_cast<DX11Renderer*>(pRenderer);

            pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);

            pDX11Renderer->getContext()->IASetInputLayout(pDX11Renderer->getBezierCurveInputLayout());
            pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().bezierCurveVS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->HSSetShader(pDX11Renderer->getShaders().bezierCurveHS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->DSSetShader(pDX11Renderer->getShaders().bezierCurveDS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().bezierCurvePS.first.Get(), nullptr, 0);

            for (unsigned int i = 0; i < mGeometry.size() + 1; i += IBezier::deBoorNumber)
            {
                unsigned int offset = i * sizeof(XMFLOAT3);
                pDX11Renderer->getContext()->IASetVertexBuffers(
                    0, 1, pDX11Renderer->mVertexBuffers.at(renderableIdx).GetAddressOf(), &getStride(),
                    &offset);

                pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
                pDX11Renderer->getContext()->HSSetShader(pDX11Renderer->getShaders().bezierCurveHS.first.Get(), nullptr,
                                                         0);
                pDX11Renderer->getContext()->DSSetShader(pDX11Renderer->getShaders().bezierCurveDS.first.Get(), nullptr,
                                                         0);
                pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);

                pDX11Renderer->getContext()->Draw(1, 0);

                if (mIsPolygon)
                {
                    pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
                    pDX11Renderer->getContext()->HSSetShader(nullptr, nullptr, 0);
                    pDX11Renderer->getContext()->DSSetShader(nullptr, nullptr, 0);
                    pDX11Renderer->getContext()->GSSetShader(
                        pDX11Renderer->getShaders().bezierCurveBorderGS.first.Get(), nullptr, 0);
                    pDX11Renderer->mGlobalCB.color = Colors::Green;

                    pDX11Renderer->updateCB(pDX11Renderer->mGlobalCB);

                    pDX11Renderer->getContext()->Draw(1, 0);
                }
            }
        }

        bool mIsPolygon{};

        void insertDeBoorPointId(const Id id)
        {
            mControlPointIds.push_back(id);
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
            : IBezier{selectedRenderableIds, "BezierC0 " + std::to_string(counter++), color}, mpRenderer{pRenderer}
        {
        }

        void generateGeometry() override
        {
            IRenderable::generateGeometry();

            unsigned idx = 0;

            for (const auto id : mControlPointIds)
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
            : IBezier{selectedRenderableIds, "BezierC2 " + std::to_string(counter++), color}, mpRenderer{pRenderer}
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

            for (const auto id : mControlPointIds)
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
            : IBezier{selectedRenderableIds, "InterpolatedBezierC2 " + std::to_string(counter++), color},
              mpRenderer{pRenderer}
        {
        }

        void generateGeometry() override
        {
            IRenderable::generateGeometry();

            std::vector<XMVECTOR> deBoorPositions;
            deBoorPositions.reserve(mControlPointIds.size());

            if (mControlPointIds.size() == 0)
            {
                return;
            }

            auto pFirstRenderable = mpRenderer->getRenderable(*mControlPointIds.begin());
            if (pFirstRenderable != nullptr)
            {
                deBoorPositions.push_back(pFirstRenderable->getGlobalPos());
            }

            for (auto it = mControlPointIds.begin() + 1; it != mControlPointIds.end();)
            {
                if (*(it - 1) == *it)
                {
                    it = mControlPointIds.erase(it);
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
        int u = 1;
        int v = 1;
        bool isWrapped = false;
        bool isC2 = false;
    };

    class IBezierPatch : public IRenderable, public IControlPointBased
    {
    protected:
        IBezierPatch(std::string&& tag, FXMVECTOR pos, IRenderer* pRenderer, const BezierPatchCreator&& bezierPatchCreator)
            : IRenderable{std::move(tag), pos}, mpRenderer{pRenderer}, mBezierPatchCreator{std::move(bezierPatchCreator)}
        {

        }

        const unsigned int& getStride() const override
        {
            static constexpr unsigned int stride = 16 * sizeof(XMFLOAT3);
            return stride;
        }

        void draw(class IRenderer* pRenderer, unsigned long long int renderableIdx) override
        {
            DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(pRenderer);

            pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);

            pDX11Renderer->getContext()->IASetInputLayout(pDX11Renderer->getBezierPatchInputLayout());
            pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().bezierPatchC0VS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->HSSetShader(pDX11Renderer->getShaders().bezierPatchC0HS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->DSSetShader(pDX11Renderer->getShaders().bezierPatchC0DS.first.Get(), nullptr, 0);
            pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
            pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().bezierPatchC0PS.first.Get(), nullptr, 0);

            D3D11_RASTERIZER_DESC rasterDesc;
            ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));

            rasterDesc.FillMode = D3D11_FILL_WIREFRAME; // Set fill mode to wireframe
            rasterDesc.CullMode = D3D11_CULL_NONE;      // No culling
            rasterDesc.FrontCounterClockwise = false;   // Front face is clockwise
            rasterDesc.DepthBias = 0;
            rasterDesc.DepthBiasClamp = 0.0f;
            rasterDesc.SlopeScaledDepthBias = 0.0f;
            rasterDesc.DepthClipEnable = true;        // Enable depth clipping
            rasterDesc.ScissorEnable = false;         // Disable scissor test
            rasterDesc.MultisampleEnable = false;     // Disable multisampling
            rasterDesc.AntialiasedLineEnable = false; // Disable line anti-aliasing

            ID3D11RasterizerState* pRasterState = nullptr;
            HR(pDX11Renderer->getDevice()->CreateRasterizerState(&rasterDesc, &pRasterState));
            pDX11Renderer->getContext()->RSSetState(pRasterState);
            pRasterState->Release();

            pDX11Renderer->getContext()->Draw(1, 0);
        }

        IRenderer* const mpRenderer;
        const BezierPatchCreator mBezierPatchCreator;
    };

    class BezierPatchC0 final : public IBezierPatch
    {
        COUNTER();

    public:
        BezierPatchC0(const BezierPatchCreator&& bezierPatchCreator, FXMVECTOR pos, IRenderer* pRenderer)
            : IBezierPatch{"BezierPatchC0 " + std::to_string(counter++), pos, pRenderer, std::move(bezierPatchCreator)}
        {
        }

        ~BezierPatchC0()
        {
            for (Id id : mControlPointIds)
            {
               auto pPoint = static_cast<Point*>(mpRenderer->getRenderable(id));
               pPoint->setDeletable(true);
            }
        }

        void updateControlPoints()
        {
            mGeometry.clear();

            for (auto pControlPointId : mControlPointIds)
            {
                auto pRenderable = mpRenderer->getRenderable(pControlPointId);

                XMFLOAT3 pos;
                XMStoreFloat3(&pos, pRenderable->getGlobalPos());
                mGeometry.push_back(pos);
            }
        }

        void regenerateData() override
        {
            IRenderable::regenerateData();

            if (!mBezierPatchCreator.isWrapped)
            {
                std::vector<std::unique_ptr<Point>> controlPoints =
                    createControlPointsForFlatSurface(getGlobalPos(), mBezierPatchCreator.u, mBezierPatchCreator.v);

                mControlPointIds.reserve(controlPoints.size());

                for (auto& pControlPoint : controlPoints)
                {
                    mControlPointIds.push_back(pControlPoint->id);

                    pControlPoint->setDeletable(false);
                    pControlPoint->regenerateData();

                    XMFLOAT3 pos;
                    XMStoreFloat3(&pos, pControlPoint->getGlobalPos());
                    mGeometry.push_back(pos);

                    mpRenderer->addRenderable(std::move(pControlPoint));
                }
            }
            else
            {
                ASSERT(false);
            }
        }

        void generateGeometry() override
        {
            IRenderable::generateGeometry();
        }

    private:
        std::vector<std::unique_ptr<Point>> createControlPointsForFlatSurface(FXMVECTOR pos_, unsigned int u, unsigned int v)
        {
            static constexpr XMVECTOR unitX{1.f, 0.f, 0.f, 0.f};
            static constexpr XMVECTOR unitZ{0.f, 0.f, 1.f, 0.f};

            std::vector<std::unique_ptr<Point>> controlPoints;

            XMVECTOR startPos = pos_ - u  / 2.f * unitX - v / 2.f * unitZ;

            XMVECTOR pointStepU = 1.f / 3 * unitX;
            XMVECTOR pointStepV = 1.f / 3 * unitZ;

            unsigned int pointsCountU = u * 3 + 1;
            unsigned int pointsCountV = v * 3 + 1;

            for (unsigned int i = 0; i < pointsCountV; ++i)
            {
                for (unsigned int j = 0; j < pointsCountU; ++j)
                {
                    XMVECTOR pos = startPos + static_cast<float>(i) * pointStepV + static_cast<float>(j) * pointStepU;
                    auto pPoint = std::make_unique<Point>(pos, 0.01f);
                    controlPoints.push_back(std::move(pPoint));
                }
            }

            return controlPoints;
        }

        std::vector<std::unique_ptr<Point>> createControlPointsForCylinder(XMVECTOR pos, unsigned int u, unsigned int v)
        {
            std::vector<std::unique_ptr<Point>> controlPoints;

            //float cylinderRadius = u / (2.0f * std::numbers::pi);

            //XMVECTOR cylinderMainAxis = Vector3::UnitZ;

            //float pointStepV = 1.f / 3 * cylinderMainAxis;

            //float patchPivotAngle = 2.0f * std::numbers::pi / u;

            //float ca = cosf(patchPivotAngle), sa = sinf(patchPivotAngle);
            //auto scaleFactor = 4.0f / 3.0f * tanf(0.25f * patchPivotAngle);

            //auto radiusVector = cylinderRadius * Vector3::UnitY;

            //for (int i = 0; i < 3 * v + 1; i++)
            //{
            //    auto startingPosition = pos - 0.5f * v * cylinderMainAxis + i * pointStepV;

            //    for (int j = 0; j < u; j++)
            //    {
            //        auto previousRadiusVector = radiusVector;
            //        auto tangent = previousRadiusVector.Cross(cylinderMainAxis);
            //        auto tangentNormalized = tangent;
            //        tangentNormalized.Normalize();

            //        auto nextRadiusVector = ca * previousRadiusVector + sa * tangent;
            //        auto nextTangent = nextRadiusVector.Cross(cylinderMainAxis);
            //        nextTangent.Normalize();

            //        auto position0 = startingPosition + previousRadiusVector;
            //        auto position1 = position0 + scaleFactor * tangentNormalized;
            //        auto position2 = startingPosition + nextRadiusVector - scaleFactor * nextTangent;

            //        auto point0 = scene.CreatePoint(position0);
            //        auto point1 = scene.CreatePoint(position1);
            //        auto point2 = scene.CreatePoint(position2);

            //        controlPoints.push_back(std::dynamic_pointer_cast<Point>(point0.lock()));
            //        controlPoints.push_back(std::dynamic_pointer_cast<Point>(point1.lock()));
            //        controlPoints.push_back(std::dynamic_pointer_cast<Point>(point2.lock()));

            //        radiusVector = nextRadiusVector;
            //    }
            //}

            return controlPoints;
        }
    };

    inline void reloadCounters()
    {
        // clang-format off
        Point::counter = Torus::counter =
        BezierC0::counter = BezierC2::counter = InterpolatedBezierC2::counter =
        BezierPatchC0::counter = 1;
        // clang-format on
    }
}