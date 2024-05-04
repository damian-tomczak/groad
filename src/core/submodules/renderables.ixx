module;

#include "mg.hpp"

export module core.renderables;
import std.core;
import dx11renderer;

using namespace DirectX;

export
{
    class Point : public IRenderable
    {
        inline static unsigned counter = 1;

    public:
        Point(XMVECTOR pos, float radius = 0.1f, int segments = 50, bool wo = true)
            : IRenderable{pos, std::format("Point {}", wo ? counter++ : 0).c_str()}, mRadius(radius), mSegments(segments)
        {
        }

        float mRadius;
        int mSegments;

        void generateGeometry() override
        {
            mGeometry.clear();
            for (int i = 0; i <= mSegments; ++i)
            {
                float theta = (i / static_cast<float>(mSegments)) * std::numbers::pi_v<float>;
                for (int j = 0; j <= mSegments; ++j)
                {
                    float phi = (j / static_cast<float>(mSegments)) * 2 * std::numbers::pi_v<float>;
                    float x = mRadius * std::sin(theta) * std::cos(phi);
                    float y = mRadius * std::sin(theta) * std::sin(phi);
                    float z = mRadius * std::cos(theta);
                    mGeometry.emplace_back(x, y, z);
                }
            }
        }

        void generateTopology() override
        {
            mTopology.clear();
            for (int i = 0; i < mSegments; ++i)
            {
                for (int j = 0; j < mSegments; ++j)
                {
                    unsigned first = (i * (mSegments + 1)) + j;
                    unsigned second = first + mSegments + 1;

                    mTopology.insert(mTopology.end(), {first, second, first + 1});
                    mTopology.insert(mTopology.end(), {second, second + 1, first + 1});
                }
            }
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

    class Bezier : public IRenderable
    {
    public:
        Bezier(const std::unordered_set<IRenderable::Id>& selectedRenderableIds, std::string_view tag)
            : IRenderable{XMVectorZero(), tag}, mControlPointRenderableIds{selectedRenderableIds}
        {
        }

        static constexpr unsigned controlPointsNumber = 4;

        bool mIsPolygon{};

        const std::unordered_set<IRenderable::Id>& getControlPointIds() const
        {
            return mControlPointRenderableIds;
        }

        void insertControlPoint(const IRenderable::Id id)
        {
            mControlPointRenderableIds.insert(id);
            generateGeometry();
        }

    protected:
        std::unordered_set<IRenderable::Id> mControlPointRenderableIds;
    };

    class BezierC0 final : public Bezier
    {
        inline static unsigned counter = 1;

        const IRenderer* mpRenderer;

    public:
        BezierC0(const std::unordered_set<IRenderable::Id>& selectedRenderableIds, const IRenderer* pRenderer)
            : Bezier{selectedRenderableIds, std::format("BezierC0 {}", counter++).c_str()}, mpRenderer{pRenderer}
        {
        }

        void generateGeometry() override
        {
            mGeometry.clear();

            unsigned idx = 0;

            for (const auto id : mControlPointRenderableIds)
            {
                if ((idx != 0) && (idx % controlPointsNumber == 0))
                {
                    const XMFLOAT3 previousPoint = mGeometry.at(mGeometry.size() - 1);
                    mGeometry.push_back(previousPoint);
                    idx++;
                }

                const XMVECTOR positionVec = mpRenderer->getRenderable(id)->mWorldPos;
                XMFLOAT3 position;
                XMStoreFloat3(&position, positionVec);
                mGeometry.push_back(position);

                idx++;
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