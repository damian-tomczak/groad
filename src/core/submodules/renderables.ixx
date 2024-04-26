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
                    mGeometry.insert(mGeometry.end(), {x, y, z});
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
                    mGeometry.insert(mGeometry.end(), {x, y, z});
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
        Bezier(std::string_view tag) : IRenderable{XMVectorZero(), tag}
        {
        }
    };

    class BezierC0 final : public Bezier
    {
        inline static unsigned counter = 1;

        const IRenderer* mpRenderer;

    public:
        BezierC0(const std::vector<IRenderable::Id>& selectedRenderableIds, const IRenderer* pRenderer)
            : Bezier{std::format("BezierC0 {}", counter++).c_str()}, mpRenderer{pRenderer}
        {
            for (const auto id : selectedRenderableIds)
            {
                const XMVECTOR positionVec = pRenderer->getRenderable(id)->mWorldPos;
                XMFLOAT3 position;
                XMStoreFloat3(&position, positionVec);
                mGeometry.insert(mGeometry.end(), {position.x, position.y, position.z});
            }
        }
    };
}