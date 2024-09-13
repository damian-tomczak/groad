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

namespace
{
inline constexpr XMVECTOR unitX{1.f, 0.f, 0.f, 1.f};
inline constexpr XMVECTOR unitY{0.f, 1.f, 0.f, 1.f};
inline constexpr XMVECTOR unitZ{0.f, 0.f, 1.f, 1.f};

inline constexpr XMVECTOR uAxis = unitX;
inline constexpr XMVECTOR vAxis = unitZ;
}

export
{
    class CounterReloader;

    class Point : public IRenderable
    {
        COUNTER()

        static constexpr float defaultRadius = 0.1f;
        static constexpr int defaultNumberOfSegments = 50;

    public:
        Point(FXMVECTOR pos = XMVectorZero(), float radius = defaultRadius, int segments = defaultNumberOfSegments)
            : IRenderable{ "Point " + std::to_string(counter++), pos }, mRadius(radius), mSegments(segments)
        {}

        const unsigned int& getStride() const override { static constexpr unsigned int stride = sizeof(XMFLOAT3); return stride; }
        void draw(class IRenderer* pRenderer, unsigned long long int renderableIdx) override;
        static void drawPrimitive(IRenderer* pRenderer, XMVECTOR pos, float radius = defaultRadius, int segments = defaultNumberOfSegments);

        static std::vector<XMFLOAT3> produceGeometry(float radius, int segments);
        static std::vector<unsigned int> produceTopology(int segments);

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

        void applySerializerData(const MG1::Point& serialPoint)
        {
            IRenderable::applySerializerData(serialPoint);

            mWorldPos = XMVECTOR{serialPoint.position.x, serialPoint.position.y, serialPoint.position.z, 1.f};
        }

        float mRadius;
        int mSegments;
    };

    class Torus : public IRenderable
    {
        COUNTER();

    public:
        Torus(FXMVECTOR pos = XMVectorZero(), float majorRadius = 0.7f, float minorRadius = 0.2f,
              int majorSegments = 100,
              int minorSegments = 20)
            : IRenderable{"Torus " + std::to_string(counter++), pos}, mMajorRadius(majorRadius),
              mMinorRadius(minorRadius), mMajorSegments(majorSegments), mMinorSegments(minorSegments)
        {
        }

        const unsigned int& getStride() const override { static constexpr unsigned int stride = sizeof(XMFLOAT3); return stride; }

        void draw(class IRenderer* pRenderer, unsigned long long int renderableIdx);

        void generateGeometry();
        void generateTopology();

        void applySerializerData(const MG1::Torus& serialTorus);

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
        {}

    public:
        std::vector<Id> mControlPointIds;
    };

    class IBezierCurve : public IRenderable, public IControlPointBased
    {
    public:
        IBezierCurve(const std::vector<Id>& selectedRenderableIds, std::string&& tag, Color color)
            : IRenderable{std::move(tag)}, IControlPointBased{selectedRenderableIds}
        {}

        static constexpr unsigned deBoorNumber = 4;

        const unsigned int& getStride() const override { static constexpr unsigned int stride = 4 * sizeof(XMFLOAT3); return stride; }

        void draw(class IRenderer* pRenderer, unsigned long long int renderableIdx) override;

        void insertDeBoorPointId(const Id id)
        {
            mControlPointIds.push_back(id);
            generateGeometry();
        }

        void applySerializerData(const MG1::Bezier& serialBezier)
        {
            IRenderable::applySerializerData(serialBezier);

            for (MG1::PointRef pointRef : serialBezier.controlPoints)
            {
                mControlPointIds.push_back(pointRef.GetId());
            }

            regenerateData();
        }

        bool isScalable() override { return true; }

        bool mIsPolygon{};
    };

    class BezierCurveC0 final : public IBezierCurve
    {
        COUNTER();

        const IRenderer* mpRenderer;

    public:
        BezierCurveC0(const std::vector<Id>& selectedRenderableIds, const IRenderer* pRenderer, Color color = defaultColor)
            : IBezierCurve{selectedRenderableIds, "BezierCurveC0 " + std::to_string(counter++), color}, mpRenderer{pRenderer}
        {}

        void generateGeometry() override;
    };

    class BezierCurveC2 final : public IBezierCurve
    {
        COUNTER();

        const IRenderer* mpRenderer;

    public:
        BezierCurveC2(const std::vector<Id>& selectedRenderableIds, const IRenderer* pRenderer, Color color = defaultColor)
            : IBezierCurve{selectedRenderableIds, "BezierCurveC2 " + std::to_string(counter++), color}, mpRenderer{pRenderer}
        {}

        void regenerateData() override
        {
            IRenderable::regenerateData();

            generateBernsteinPoints();
        }

        void generateGeometry() override;

        std::vector<XMFLOAT3>& getBernsteinPositions() { return mGeometry; }

        void updateBernstein(int bernsteinIdx, XMVECTOR delta)
        {
            const int bSplineId = (bernsteinIdx + 4 + 2) / 4;
            
            const Id bSplinePointId = mControlPointIds.at(bSplineId);
            IRenderable* bSplinePoint = mpRenderer->getRenderable(bSplinePointId);

            XMVECTOR farPoint;
            switch (bernsteinIdx % 4)
            {
            case 0:
            {
                const Id bSplineBeforePointId = mControlPointIds.at(bSplineId - 1);
                const Id bSplineNextPointId = mControlPointIds.at(bSplineId + 1);
                const IRenderable* bSplineBeforePoint = mpRenderer->getRenderable(bSplineBeforePointId);
                const IRenderable* bSplineNextPoint = mpRenderer->getRenderable(bSplineNextPointId);
                farPoint = (bSplineBeforePoint->mWorldPos + bSplineNextPoint->mWorldPos) / 2.f;
                break;
            }
            case 1:
            {
                const Id bSplineNextPointId = mControlPointIds.at(bSplineId + 1);
                const IRenderable* bSplineNextPoint = mpRenderer->getRenderable(bSplineNextPointId);
                farPoint = bSplineNextPoint->mWorldPos;
                break;
            }
            case 2:
            {
                const Id bSplineBeforePointId = mControlPointIds.at(bSplineId - 1);
                const IRenderable* bSplineBeforePoint = mpRenderer->getRenderable(bSplineBeforePointId);
                farPoint = bSplineBeforePoint->mWorldPos;
                break;
            }
            case 3:
            {
                const Id bSplineBeforePointId = mControlPointIds.at(bSplineId - 1);
                const Id bSplineNextPointId = mControlPointIds.at(bSplineId + 1);
                const IRenderable* bSplineBeforePoint = mpRenderer->getRenderable(bSplineBeforePointId);
                const IRenderable* bSplineNextPoint = mpRenderer->getRenderable(bSplineNextPointId);
                farPoint = (bSplineBeforePoint->mWorldPos + bSplineNextPoint->mWorldPos) / 2.f;
                break;
            }
            // TODO: investigate it
            //case 3:
            //{
            //    const Id bSplineNextPointId = mControlPointIds.at(bSplineId + 1);
            //    const Id bSplineNextNextPointId = mControlPointIds.at(bSplineId + 2);
            //    const IRenderable* bSplineNextPoint = mpRenderer->getRenderable(bSplineNextPointId);
            //    const IRenderable* bSplineNextNextPoint = mpRenderer->getRenderable(bSplineNextNextPointId);
            //    farPoint = (bSplineNextPoint->getGlobalPos() + bSplineNextNextPoint->getGlobalPos()) / 2.f;
            //    break;
            //}
            }

            // TODO: WO
            XMVECTOR bernsteinPos = XMLoadFloat3(&mGeometry.at(bernsteinIdx));
            auto var1 = XMVector3Length(bSplinePoint->mWorldPos - farPoint);
            float var1length;
            XMStoreFloat(&var1length, var1);
            auto var2 = XMVector3Length(bernsteinPos - delta - farPoint);
            XMVECTOR length = (var1length == 0.f ? XMVECTOR{0.001f, 0.001f, 0.001f, 0.001f} : var1) / var2;
            float ratio;
            XMStoreFloat(&ratio, length);

            XMVECTOR d = delta * ratio;
            bSplinePoint->mWorldPos += d;
        }

    private:
        void generateBernsteinPoints();
    };

    class InterpolatedBezierCurveC2 final : public IBezierCurve
    {
        COUNTER();

        const IRenderer* mpRenderer;

    public:
        InterpolatedBezierCurveC2(const std::vector<Id>& selectedRenderableIds, const IRenderer* pRenderer, Color color = defaultColor)
            : IBezierCurve{selectedRenderableIds, "InterpolatedBezierCurveC2 " + std::to_string(counter++), color},
              mpRenderer{pRenderer}
        {}

        void generateGeometry() override;

    private:
        std::tuple<std::vector<float>, std::vector<float>, std::vector<XMVECTOR>> createSystemOfEquations(const std::vector<XMVECTOR>& points);
        std::vector<XMVECTOR> solveSplineSystem(const std::vector<float>& alfa, const std::vector<float>& beta, std::vector<XMVECTOR>& R);
        std::vector<XMVECTOR> getBezierPoints(const std::vector<XMVECTOR>& cs, const std::vector<XMVECTOR>& points);
    };

    struct BezierSurfaceCreator final
    {
        unsigned int u = 1;
        unsigned int v = 1;
        bool isWrapped = false;
        bool isC2 = false;
        float patchSizeU = 1.f, patchSizeV = 1.f;
    };

    class IBezierSurface : public IRenderable, public IControlPointBased
    {
    public:
        void rotateControlPoints(float pitchoff, float yawoff, float rolloff, float scaleoff)
        {
            int count = 0;
            XMVECTOR sum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
            for (auto& bezierPatch : mBezierPatches)
            {
                for (auto selectedRenderableId : bezierPatch.controlPointIds)
                {
                    const auto pRenderable = mpRenderer->getRenderable(selectedRenderableId);
                    sum = XMVectorAdd(sum, pRenderable->getGlobalPos());
                    count++;
                }
            }

            float numPositions = static_cast<float>(count);
            XMVECTOR pivotPos = XMVectorScale(sum, 1.0f / numPositions);

            for (auto& bezierPatch : mBezierPatches)
            {
                for (auto renderableSelectedId : bezierPatch.controlPointIds)
                {
                    IRenderable* const pRenderable = mpRenderer->getRenderable(renderableSelectedId);

                    XMMATRIX model = XMMatrixIdentity();

                    XMMATRIX localScaleMat = XMMatrixScalingFromVector(pRenderable->mScale);
                    XMMATRIX pivotScaleMat = XMMatrixIdentity();

                    XMMATRIX pitchRotationMatrix = XMMatrixRotationX(pRenderable->mPitch);
                    XMMATRIX yawRotationMatrix = XMMatrixRotationY(pRenderable->mYaw);
                    XMMATRIX rollRotationMatrix = XMMatrixRotationZ(pRenderable->mRoll);

                    XMMATRIX localRotationMat = pitchRotationMatrix * yawRotationMatrix * rollRotationMatrix;
                    XMMATRIX pivotRotationMat = XMMatrixIdentity();

                    XMMATRIX translationToOrigin = XMMatrixIdentity();
                    XMMATRIX translationBack = XMMatrixIdentity();

                    //if (mCtx.selectedRenderableIds.contains(pRenderable->getId()))
                    //{
                        XMMATRIX pivotPitchRotationMat = XMMatrixRotationX(pitchoff);
                        XMMATRIX pivotYawRotationMat = XMMatrixRotationY(yawoff);
                        XMMATRIX pivotRollRotationMat = XMMatrixRotationZ(rolloff);

                        pivotRotationMat = pivotPitchRotationMat * pivotYawRotationMat * pivotRollRotationMat;

                        pivotScaleMat = XMMatrixScaling(scaleoff, scaleoff, scaleoff);

                        translationToOrigin = XMMatrixTranslationFromVector(-(pivotPos - pRenderable->mWorldPos));
                        translationBack = XMMatrixTranslationFromVector((pivotPos - pRenderable->mWorldPos));
                    //}

                    XMMATRIX worldTranslation = XMMatrixTranslationFromVector(pRenderable->mWorldPos);

                    model = localScaleMat * localRotationMat *
                        translationToOrigin * pivotScaleMat * pivotRotationMat * translationBack *
                        worldTranslation;


                    pRenderable->mWorldPos = model.r[3];

                    const XMMATRIX scaleMat = localScaleMat * pivotScaleMat;
                    pRenderable->mScale = XMVECTOR{ XMVectorGetX(scaleMat.r[0]), XMVectorGetY(scaleMat.r[1]), XMVectorGetZ(scaleMat.r[2]), 1.f };

                    const XMMATRIX rotationMat =
                        localRotationMat * translationToOrigin * pivotRotationMat * translationBack;
                    const auto [pitch, yaw, roll] = mg::getPitchYawRollFromRotationMat(rotationMat);

                    pRenderable->mPitch = pitch;
                    pRenderable->mYaw = yaw;
                    pRenderable->mRoll = roll;
                }
            }

            updateControlPoints();
        }

    protected:
        static constexpr float controlPointSize = 0.01f;
        static constexpr unsigned int numberOfControlPoints = 16;

        IBezierSurface(FXMVECTOR pos, std::string&& tag, IRenderer* pRenderer, const BezierSurfaceCreator&& bezierPatchCreator)
            : IRenderable{std::move(tag), pos}, mpRenderer{pRenderer}, mBezierPatchCreator{std::move(bezierPatchCreator)}
        {}
        ~IBezierSurface();
        
        struct BezierPatch
        {
            std::vector<Id> controlPointIds;
        };

        const unsigned int& getStride() const override { static constexpr unsigned int stride = numberOfControlPoints * sizeof(XMFLOAT3); return stride; }

        void regenerateData() override;
        void draw(class IRenderer* pRenderer, unsigned long long int renderableIdx) override;
        void updateControlPoints();

        virtual std::vector<std::unique_ptr<Point>> createControlPointsForFlatSurface(FXMVECTOR pos_, unsigned int u, unsigned int v, float patchSizeU, float patchSizeV) = 0;
        virtual std::vector<std::unique_ptr<Point>> createControlPointsForCylinder(FXMVECTOR pos_, unsigned int u, unsigned int v, float patchSizeU, float patchSizeV) = 0;

        IRenderer* const mpRenderer;
        const BezierSurfaceCreator mBezierPatchCreator;
        std::vector<BezierPatch> mBezierPatches;
    };

    class BezierSurfaceC0 final : public IBezierSurface
    {
        COUNTER();

    public:
        BezierSurfaceC0(FXMVECTOR pos, const BezierSurfaceCreator&& bezierPatchCreator, IRenderer* pRenderer)
            : IBezierSurface{pos, "BezierSurfaceC0 " + std::to_string(counter++), pRenderer, std::move(bezierPatchCreator)}
        {}

        void regenerateData() override;

        void draw(class IRenderer* pRenderer, unsigned long long int renderableIdx) override;

        std::vector<std::unique_ptr<Point>> createControlPointsForFlatSurface(FXMVECTOR pos_, unsigned int u, unsigned int v, float patchSizeU, float patchSizeV);
        std::vector<std::unique_ptr<Point>> createControlPointsForCylinder(FXMVECTOR pos, unsigned int u, unsigned int v, float patchSizeU, float patchSizeV);

        void applySerializerData(const MG1::BezierSurfaceC0& serialBezierSurface, std::vector<MG1::BezierPatchC0> patches)
        {
            // TODO: optimize it
            IRenderable::applySerializerData(serialBezierSurface);

            mBezierPatches.clear();
            for (MG1::BezierPatchC0& serializerPatch : patches)
            {
                BezierPatch patch;
                for (auto pointRef : serializerPatch.controlPoints)
                {
                    patch.controlPointIds.emplace_back(pointRef.GetId());
                    mControlPointIds.emplace_back(pointRef.GetId());
                }
                mBezierPatches.push_back(patch);
            }
        }
    };

    class BezierSurfaceC2 final : public IBezierSurface
    {
        COUNTER();

    public:
        BezierSurfaceC2(FXMVECTOR pos, const BezierSurfaceCreator&& bezierPatchCreator, IRenderer* pRenderer)
            : IBezierSurface{pos, "BezierSurfaceC2 " + std::to_string(counter++), pRenderer, std::move(bezierPatchCreator)}
        {}

        void regenerateData() override;

        void draw(class IRenderer* pRenderer, unsigned long long int renderableIdx) override;

        std::vector<std::unique_ptr<Point>> createControlPointsForFlatSurface(FXMVECTOR pos_, unsigned int u, unsigned int v, float patchSizeU, float patchSizeV);
        std::vector<std::unique_ptr<Point>> createControlPointsForCylinder(FXMVECTOR pos, unsigned int u, unsigned int v, float patchSizeU, float patchSizeV);

        void applySerializerData(const MG1::BezierSurfaceC2& serialBezierSurface, std::vector<MG1::BezierPatchC2> patches)
        {
            // TODO: optimize it
            IRenderable::applySerializerData(serialBezierSurface);

            mBezierPatches.clear();
            for (MG1::BezierPatchC2& serializerPatch : patches)
            {
                BezierPatch patch;
                for (auto pointRef : serializerPatch.controlPoints)
                {
                    patch.controlPointIds.emplace_back(pointRef.GetId());
                    mControlPointIds.emplace_back(pointRef.GetId());
                }
                ASSERT(patch.controlPointIds.size() == numberOfControlPoints);
                mBezierPatches.push_back(patch);
            }
        }

        bool mIsPolygon{};
    };

    inline void reloadCounters()
    {
        Point::counter = Torus::counter =
        BezierCurveC0::counter = BezierCurveC2::counter = InterpolatedBezierCurveC2::counter =
        BezierSurfaceC0::counter = BezierSurfaceC2::counter = 1;
    }
}

module :private;

void Point::draw(class IRenderer* pRenderer, unsigned long long int renderableIdx)
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

void Point::drawPrimitive(IRenderer* pRenderer, XMVECTOR pos, float radius, int segments)
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
    pDX11Renderer->getContext()->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &vStride, &offset);
    pDX11Renderer->getContext()->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().defaultVS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().defaultPS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->HSSetShader(nullptr, nullptr, 0);
    pDX11Renderer->getContext()->DSSetShader(nullptr, nullptr, 0);
    pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
    pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pDX11Renderer->getContext()->DrawIndexed(static_cast<UINT>(topology.size()), 0, 0);
}

std::vector<XMFLOAT3> Point::produceGeometry(float radius, int segments)
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

std::vector<unsigned int> Point::produceTopology(int segments)
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

            topology.insert(topology.end(), { topLeft, bottomLeft, topRight });
            topology.insert(topology.end(), { topRight, bottomLeft, bottomRight });
        }
    }

    return topology;
}

void Torus::draw(class IRenderer* pRenderer, unsigned long long int renderableIdx)
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(pRenderer);

    pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    pDX11Renderer->getContext()->IASetInputLayout(pDX11Renderer->getDefaultInputLayout());
    pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().defaultVS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->HSSetShader(nullptr, nullptr, 0);
    pDX11Renderer->getContext()->DSSetShader(nullptr, nullptr, 0);
    pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
    pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().defaultPS.first.Get(), nullptr, 0);

    pDX11Renderer->getContext()->DrawIndexed(static_cast<UINT>(getTopology().size()), 0, 0);
}

void Torus::generateGeometry()
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

void Torus::generateTopology()
{
    IRenderable::generateTopology();

    for (int i = 0; i < mMajorSegments; ++i)
    {
        for (int j = 0; j < mMinorSegments; ++j)
        {
            unsigned first = (i * (mMinorSegments + 1)) + j;
            unsigned nextInMinor = first + 1;
            unsigned nextInMajor = (first + mMinorSegments + 1) % (mMajorSegments * (mMinorSegments + 1));

            mTopology.insert(mTopology.end(), { first, nextInMinor });
            mTopology.insert(mTopology.end(), { first, nextInMajor });
        }
    }
}

void Torus::applySerializerData(const MG1::Torus& serialTorus)
{
    IRenderable::applySerializerData(serialTorus);

    mWorldPos = XMVECTOR{serialTorus.position.x, serialTorus.position.y, serialTorus.position.z, 1.f};

    mPitch = serialTorus.rotation.x;
    mYaw = serialTorus.rotation.y;
    mRoll = serialTorus.rotation.z;
    mScale = XMVECTOR{serialTorus.scale.x, serialTorus.scale.y, serialTorus.scale.z, 1.f};

    mMajorSegments = serialTorus.samples.x;
    mMinorSegments = serialTorus.samples.y;

    mMajorRadius = serialTorus.largeRadius;
    mMinorRadius = serialTorus.smallRadius;
}

void IBezierCurve::draw(class IRenderer* pRenderer, unsigned long long int renderableIdx)
{
    auto pDX11Renderer = static_cast<DX11Renderer*>(pRenderer);

    auto defaultColor = pDX11Renderer->mGlobalCB.color;

    pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);

    pDX11Renderer->getContext()->IASetInputLayout(pDX11Renderer->getBezierCurveInputLayout());
    pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().bezierCurveVS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->HSSetShader(pDX11Renderer->getShaders().bezierCurveHS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->DSSetShader(pDX11Renderer->getShaders().bezierCurveDS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
    pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().bezierCurvePS.first.Get(), nullptr, 0);

    for (unsigned int i = 0; i < mGeometry.size() + 1; i += IBezierCurve::deBoorNumber)
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
        pDX11Renderer->mGlobalCB.color = defaultColor;
        pDX11Renderer->updateCB(pDX11Renderer->mGlobalCB);

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

void BezierCurveC0::generateGeometry()
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

void BezierCurveC2::generateGeometry()
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
            mGeometry.insert(mGeometry.end(), { previousPoint3, previousPoint2, previousPoint1 });
            idx += 3;
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

void BezierCurveC2::generateBernsteinPoints()
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

void InterpolatedBezierCurveC2::generateGeometry()
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
        deBoorPositions.push_back(pFirstRenderable->mWorldPos);
    }

    constexpr float proximityThreshold = 0.01f;
    // TODO: refactor
    for (auto it = mControlPointIds.begin() + 1; it != mControlPointIds.end();)
    {
        auto previousId = it - 1;
        auto currentId = it;
        if (*previousId == *currentId)
        {
            ++it;
        }
        else
        {
            auto pRenderableCurrent = mpRenderer->getRenderable(*currentId);
            auto pRenderablePrevious = mpRenderer->getRenderable(*previousId);

            if (pRenderableCurrent != nullptr && pRenderablePrevious != nullptr)
            {
                const XMVECTOR posCurrent = pRenderableCurrent->mWorldPos;
                const XMVECTOR posPrevious = pRenderablePrevious->mWorldPos;

                XMVECTOR diff = DirectX::XMVectorSubtract(posCurrent, posPrevious);
                if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(diff)) <= proximityThreshold * proximityThreshold)
                {
                    ++it;
                }
            }

            if (pRenderableCurrent != nullptr)
            {
                const XMVECTOR posVec = pRenderableCurrent->mWorldPos;
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

std::tuple<std::vector<float>, std::vector<float>, std::vector<XMVECTOR>> InterpolatedBezierCurveC2::createSystemOfEquations(
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

    return { alfa, beta, b };
}

std::vector<XMVECTOR> InterpolatedBezierCurveC2::solveSplineSystem(const std::vector<float>& alfa, const std::vector<float>& beta,
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

std::vector<XMVECTOR> InterpolatedBezierCurveC2::getBezierPoints(const std::vector<XMVECTOR>& cs, const std::vector<XMVECTOR>& points)
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

    static const XMMATRIX powerToBernstein = XMMatrixTranspose({
        1.0f, 0.0f,        0.0f,        0.0f,
        1.0f, 1.0f / 3.0f, 0.0f,        0.0f,
        1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 0.0f,
        1.0f, 1.0f,        1.0f,        1.0f,
    });

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

IBezierSurface::~IBezierSurface()
{
    for (auto& bezierPatch : mBezierPatches)
    {
        for (auto pControlPointId : bezierPatch.controlPointIds)
        {
            auto pPoint = static_cast<Point*>(mpRenderer->getRenderable(pControlPointId));
            if (pPoint != nullptr)
            {
                ASSERT(false);
                pPoint->setDeletable(false);
            }
        }
    }
}

void IBezierSurface::regenerateData()
{
    IRenderable::regenerateData();

    if (!mBezierPatches.empty())
    {
        updateControlPoints();

        return;
    }

    std::vector<std::unique_ptr<Point>> controlPoints;
    if (!mBezierPatchCreator.isWrapped)
    {
        controlPoints =
            createControlPointsForFlatSurface(mWorldPos, mBezierPatchCreator.u, mBezierPatchCreator.v, mBezierPatchCreator.patchSizeU, mBezierPatchCreator.patchSizeV);
    }
    else
    {
        controlPoints =
            createControlPointsForCylinder(mWorldPos, mBezierPatchCreator.u, mBezierPatchCreator.v, mBezierPatchCreator.patchSizeU, mBezierPatchCreator.patchSizeV);
    }

    mControlPointIds.reserve(controlPoints.size());

    for (auto& pControlPoint : controlPoints)
    {
        pControlPoint->setDeletable(false);
        mControlPointIds.push_back(pControlPoint->getId());

        mpRenderer->addRenderable(std::move(pControlPoint));
    }
}

void IBezierSurface::draw(class IRenderer* pRenderer, unsigned long long int renderableIdx)
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(pRenderer);

    pDX11Renderer->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);

    pDX11Renderer->getContext()->IASetInputLayout(pDX11Renderer->getBezierPatchInputLayout());

    pDX11Renderer->setWireframeRaster();

    for (unsigned int i = 0; i < mBezierPatchCreator.u * mBezierPatchCreator.v; i++)
    {
        unsigned int offset = sizeof(XMFLOAT3) * i * numberOfControlPoints;

        pDX11Renderer->getContext()->IASetVertexBuffers(
            0, 1, pDX11Renderer->mVertexBuffers.at(renderableIdx).GetAddressOf(), &getStride(), &offset);
        pDX11Renderer->getContext()->Draw(1, 0);
    }

    pDX11Renderer->setSolidRaster();
}

void IBezierSurface::updateControlPoints()
{
    mGeometry.clear();

    for (auto& bezierPatch : mBezierPatches)
    {
        for (auto controlPointId : bezierPatch.controlPointIds)
        {
            auto pRenderable = mpRenderer->getRenderable(controlPointId);

            XMFLOAT3 pos;
            XMStoreFloat3(&pos, pRenderable->mWorldPos);
            mGeometry.push_back(pos);
        }
    }
}

void BezierSurfaceC0::regenerateData()
{
    IBezierSurface::regenerateData();

    if (!mBezierPatches.empty())
    {
        return;
    }

    if (!mBezierPatchCreator.isWrapped)
    {
        unsigned int pointsCountU = mBezierPatchCreator.u * 3 + 1;

        for (unsigned int i = 0; i < mBezierPatchCreator.v; ++i)
        {
            for (unsigned int j = 0; j < mBezierPatchCreator.u; ++j)
            {
                std::vector<Id> patchControlPoints;

                unsigned int startPointU = j * 3;
                unsigned int startPointV = i * 3;

                for (unsigned int point = 0; point < numberOfControlPoints; ++point)
                {
                    unsigned int pointU = startPointU + point % 4;
                    unsigned int pointV = startPointV + point / 4;

                    unsigned int pointIndex = pointV * pointsCountU + pointU;

                    auto pRenderable = mpRenderer->getRenderable(mControlPointIds.at(pointIndex));
                    ASSERT(pRenderable != nullptr);

                    patchControlPoints.push_back(pRenderable->getId());
                }

                mBezierPatches.emplace_back(patchControlPoints);
            }
        }
    }
    else
    {
        unsigned int pointsCountU = mBezierPatchCreator.u * 3;

        for (unsigned int i = 0; i < mBezierPatchCreator.v; ++i)
        {
            for (unsigned int j = 0; j < mBezierPatchCreator.u; ++j)
            {
                std::vector<Id> patchControlPoints;

                unsigned int startPointU = j * 3;
                unsigned int startPointV = i * 3;

                for (unsigned int point = 0; point < numberOfControlPoints; ++point)
                {
                    unsigned int pointU = (startPointU + point % 4) % pointsCountU;
                    unsigned int pointV = startPointV + point / 4;

                    unsigned int pointIndex = pointV * pointsCountU + pointU;

                    auto pRenderable = mpRenderer->getRenderable(mControlPointIds.at(pointIndex));
                    ASSERT(pRenderable != nullptr);

                    patchControlPoints.push_back(pRenderable->getId());
                }

                mBezierPatches.emplace_back(patchControlPoints);
            }
        }
    }

    updateControlPoints();
}

void BezierSurfaceC0::draw(class IRenderer* pRenderer, unsigned long long int renderableIdx)
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(pRenderer);

    pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().bezierPatchC0VS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->HSSetShader(pDX11Renderer->getShaders().bezierPatchC0HS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->DSSetShader(pDX11Renderer->getShaders().bezierPatchC0DS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);
    pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().bezierPatchC0PS.first.Get(), nullptr, 0);

    IBezierSurface::draw(pRenderer, renderableIdx);
}

std::vector<std::unique_ptr<Point>> BezierSurfaceC0::createControlPointsForFlatSurface(FXMVECTOR pos_, unsigned int u, unsigned int v, float patchSizeU, float patchSizeV)
{
    std::vector<std::unique_ptr<Point>> controlPoints;

    XMVECTOR startPos =
        pos_ - static_cast<float>(u) * patchSizeU / 2 * uAxis - static_cast<float>(v) * patchSizeV / 2 * vAxis;

    const XMVECTOR pointStepU = patchSizeU / 3 * unitX;
    const XMVECTOR pointStepV = patchSizeV / 3 * unitZ;

    unsigned int pointsCountU = u * 3 + 1;
    unsigned int pointsCountV = v * 3 + 1;

    for (unsigned int i = 0; i < pointsCountV; ++i)
    {
        for (unsigned int j = 0; j < pointsCountU; ++j)
        {
            XMVECTOR pos = startPos + static_cast<float>(i) * pointStepV + static_cast<float>(j) * pointStepU;

            auto pPoint = std::make_unique<Point>(pos, controlPointSize);
            controlPoints.push_back(std::move(pPoint));
        }
    }

    return controlPoints;
}

std::vector<std::unique_ptr<Point>> BezierSurfaceC0::createControlPointsForCylinder(FXMVECTOR pos, unsigned int u, unsigned int v, float patchSizeU, float patchSizeV)
{
    std::vector<std::unique_ptr<Point>> controlPoints;

    float cylinderRadius = u * patchSizeU / (2.0f * std::numbers::pi_v<float>);

    XMVECTOR cylinderMainAxis = unitZ;

    XMVECTOR pointStepV = patchSizeV / 3 * cylinderMainAxis;

    float patchPivotAngle = 2.0f * std::numbers::pi_v<float> / u;

    float ca = cosf(patchPivotAngle), sa = sinf(patchPivotAngle);
    float scaleFactor = 4.0f / 3.0f * tanf(0.25f * patchPivotAngle);

    XMVECTOR radiusVector = cylinderRadius * unitY;

    for (unsigned int i = 0; i < 3 * v + 1; i++)
    {
        XMVECTOR startingPosition = pos - 0.5f * v * cylinderMainAxis + static_cast<float>(i) * pointStepV;

        for (unsigned int j = 0; j < u; j++)
        {
            XMVECTOR previousRadiusVector = radiusVector;
            XMVECTOR tangent = XMVector3Cross(previousRadiusVector, cylinderMainAxis);
            XMVECTOR normalizedTangent = XMVector3Normalize(tangent);

            XMVECTOR nextRadiusVector = ca * previousRadiusVector + sa * tangent;
            XMVECTOR nextTangent = XMVector3Normalize(XMVector3Cross(nextRadiusVector, cylinderMainAxis));

            XMVECTOR position1 = startingPosition + previousRadiusVector;
            XMVECTOR position2 = position1 + scaleFactor * normalizedTangent;
            XMVECTOR position3 = startingPosition + nextRadiusVector - scaleFactor * nextTangent;

            auto pPoint1 = std::make_unique<Point>(position1, controlPointSize);
            auto pPoint2 = std::make_unique<Point>(position2, controlPointSize);
            auto pPoint3 = std::make_unique<Point>(position3, controlPointSize);

            controlPoints.emplace_back(std::move(pPoint1));
            controlPoints.emplace_back(std::move(pPoint2));
            controlPoints.emplace_back(std::move(pPoint3));

            radiusVector = nextRadiusVector;
        }
    }

    return controlPoints;
}

void BezierSurfaceC2::regenerateData()
{
    IBezierSurface::regenerateData();

    if (!mBezierPatches.empty())
    {
        return;
    }

    if (!mBezierPatchCreator.isWrapped)
    {
        unsigned int pointsCountU = mBezierPatchCreator.u + 3;

        for (unsigned int i = 0; i < mBezierPatchCreator.v; ++i)
        {
            for (unsigned int j = 0; j < mBezierPatchCreator.u; ++j)
            {
                std::vector<Id> patchControlPoints;

                unsigned int startPointU = j;
                unsigned int startPointV = i;

                for (unsigned int point = 0; point < numberOfControlPoints; ++point)
                {
                    unsigned int pointU = startPointU + point % 4;
                    unsigned int pointV = startPointV + point / 4;

                    unsigned int pointIndex = pointV * pointsCountU + pointU;

                    auto pRenderable = mpRenderer->getRenderable(mControlPointIds.at(pointIndex));
                    ASSERT(pRenderable != nullptr);

                    patchControlPoints.push_back(pRenderable->getId());
                }

                mBezierPatches.emplace_back(patchControlPoints);
            }
        }
    }
    else
    {
        unsigned int pointsCountU = mBezierPatchCreator.u;

        for (unsigned int i = 0; i < mBezierPatchCreator.v; ++i)
        {
            for (unsigned int j = 0; j < mBezierPatchCreator.u; ++j)
            {
                std::vector<Id> patchControlPoints;

                unsigned int startPointU = j;
                unsigned int startPointV = i;

                for (int point = 0; point < numberOfControlPoints; ++point)
                {
                    unsigned int pointU = (startPointU + point % 4) % pointsCountU;
                    unsigned int pointV = startPointV + point / 4;

                    unsigned int pointIndex = pointV * pointsCountU + pointU;

                    auto pRenderable = mpRenderer->getRenderable(mControlPointIds.at(pointIndex));
                    ASSERT(pRenderable != nullptr);

                    patchControlPoints.push_back(pRenderable->getId());
                }

                mBezierPatches.emplace_back(patchControlPoints);
            }
        }
    }

    updateControlPoints();
}

void BezierSurfaceC2::draw(class IRenderer* pRenderer, unsigned long long int renderableIdx)
{
    DX11Renderer* pDX11Renderer = static_cast<DX11Renderer*>(pRenderer);

    pDX11Renderer->getContext()->VSSetShader(pDX11Renderer->getShaders().bezierPatchC2VS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->PSSetShader(pDX11Renderer->getShaders().bezierPatchC2PS.first.Get(), nullptr, 0);

    if (mIsPolygon)
    {
        pDX11Renderer->getContext()->HSSetShader(nullptr, nullptr, 0);
        pDX11Renderer->getContext()->DSSetShader(nullptr, nullptr, 0);
        pDX11Renderer->getContext()->GSSetShader(pDX11Renderer->getShaders().bezierPatchC2GS.first.Get(), nullptr, 0);

        pDX11Renderer->mGlobalCB.color = Colors::Green;
        pDX11Renderer->updateCB(pDX11Renderer->mGlobalCB);

        IBezierSurface::draw(pRenderer, renderableIdx);
    }

    pDX11Renderer->getContext()->HSSetShader(pDX11Renderer->getShaders().bezierPatchC2HS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->DSSetShader(pDX11Renderer->getShaders().bezierPatchC2DS.first.Get(), nullptr, 0);
    pDX11Renderer->getContext()->GSSetShader(nullptr, nullptr, 0);

    pDX11Renderer->mGlobalCB.color = Colors::White;
    pDX11Renderer->updateCB(pDX11Renderer->mGlobalCB);

    IBezierSurface::draw(pRenderer, renderableIdx);
}

std::vector<std::unique_ptr<Point>> BezierSurfaceC2::createControlPointsForFlatSurface(FXMVECTOR pos_, unsigned int u, unsigned int v, float patchSizeU, float patchSizeV)
{
    std::vector<std::unique_ptr<Point>> controlPoints;

    XMVECTOR startPos =
        pos_ - (static_cast<float>(u) + 2) * patchSizeU / 2 * uAxis - (static_cast<float>(v) + 2) * patchSizeV / 2 * vAxis;

    XMVECTOR pointStepU = patchSizeU * uAxis;
    XMVECTOR pointStepV = patchSizeV * vAxis;

    unsigned int pointsCountU = u + 3;
    unsigned int pointsCountV = v + 3;

    for (unsigned int i = 0; i < pointsCountV; ++i)
    {
        for (unsigned int j = 0; j < pointsCountU; ++j)
        {
            XMVECTOR pos = startPos + static_cast<float>(i) * pointStepV + static_cast<float>(j) * pointStepU;
            auto pPoint = std::make_unique<Point>(pos, controlPointSize);
            controlPoints.push_back(std::move(pPoint));
        }
    }

    return controlPoints;
}

std::vector<std::unique_ptr<Point>> BezierSurfaceC2::createControlPointsForCylinder(FXMVECTOR pos, unsigned int u, unsigned int v, float patchSizeU, float patchSizeV)
{
    std::vector<std::unique_ptr<Point>> controlPoints;

    float cylinderRadius = u * patchSizeU / (2.0f * std::numbers::pi_v<float>);

    XMVECTOR cylinderMainAxis = unitZ;

    XMVECTOR pointStepV = patchSizeV * cylinderMainAxis;

    float patchPivotAngle = 2.0f * std::numbers::pi_v<float> / u;

    float ca = cosf(patchPivotAngle), sa = sinf(patchPivotAngle);

    unsigned int pointsCountU = u;
    unsigned int pointsCountV = v + 3;

    XMVECTOR radiusVector = cylinderRadius * unitY;

    for (unsigned int i = 0; i < pointsCountV; i++)
    {
        XMVECTOR startPos = pos + (i - 0.5f * pointsCountV) * pointStepV;

        for (unsigned int j = 0; j < pointsCountU; ++j)
        {
            auto pPoint = std::make_unique<Point>(startPos + radiusVector, controlPointSize);
            controlPoints.emplace_back(std::move(pPoint));

            XMVECTOR tangent = XMVector3Cross(radiusVector, cylinderMainAxis);

            radiusVector = ca * radiusVector + sa * tangent;
        }
    }

    return controlPoints;
}