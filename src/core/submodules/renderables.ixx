module;

#include "mg.hpp"

export module core.renderables;
import std.core;
import dx11renderer;

using namespace DirectX;

export class Point : public Renderable
{
    inline static unsigned counter{};

    std::vector<float> mGeometry;
    std::vector<unsigned> mTopology;

public:
    Point(XMVECTOR pos, float radius = 0.5f, int segments = 50)
        : Renderable{pos, std::format("Point {}", counter++).c_str()}, mRadius(radius), mSegments(segments)
    {

    }

    float mRadius;
    int mSegments;

    const std::vector<float>& getGeometry() const override
    {
        return mGeometry;
    }

    const std::vector<unsigned>& getTopology() const override
    {
        return mTopology;
    }

    virtual void generateGeometry() override
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

    virtual void generateTopology() override
    {
        mTopology.clear();
        for (int i = 0; i < mSegments; ++i)
        {
            for (int j = 0; j < mSegments; ++j)
            {
                unsigned first = i * (mSegments + 1) + j;
                unsigned nextInSegment = first + 1;
                unsigned nextRow = first + (mSegments + 1);
                unsigned nextRowNextInSegment = nextRow + 1;

                mTopology.insert(mTopology.end(), {first, nextInSegment, nextRow});
                mTopology.insert(mTopology.end(), {nextInSegment, nextRowNextInSegment, nextRow});
            }
        }
    }
};

export class Torus : public Renderable
{
    inline static unsigned counter{};

public:
    Torus(XMVECTOR pos, float majorRadius = 0.7f, float minorRadius = 0.2f, int majorSegments = 100, int minorSegments = 20)
        : Renderable{pos, std::format("Torus {}", counter++).c_str()},
        mMajorRadius(majorRadius),
        mMinorRadius(minorRadius),
        mMajorSegments(majorSegments),
        mMinorSegments(minorSegments)
    {

    }

    float mMajorRadius;
    float mMinorRadius;
    int mMajorSegments;
    int mMinorSegments;

    const std::vector<float>& getGeometry() const override
    {
        return mGeometry;
    }

    const std::vector<unsigned>& getTopology() const override
    {
        return mTopology;
    }

    virtual void generateGeometry() override
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

    virtual void generateTopology() override
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