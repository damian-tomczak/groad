module;

#include <DirectXMath.h>

export module core.renderables;
import std.core;
import core.renderer;

using namespace DirectX;

export class Point : public Renderable
{
    inline static unsigned counter{};

public:
    Point() : Renderable{std::format("Point {}", counter++).c_str()}
    {

    }
};

export class Torus : public Renderable
{
    inline static unsigned counter{};

public:
    Torus(float majorRadius, float minorRadius, int majorSegments, int minorSegments)
        : Renderable{std::format("Torus {}", counter++).c_str()}, mMajorRadius(majorRadius), mMinorRadius(minorRadius),
          mMajorSegments(majorSegments),
          mMinorSegments(minorSegments)
    {
        generateGeometry();
        generateTopology();
    }

    float mMajorRadius;
    float mMinorRadius;
    int mMajorSegments;
    int mMinorSegments;
    bool mIsDataChanged{};

    const std::vector<float>& getGeometry() const override
    {
        return mGeometry;
    }

    const std::vector<unsigned>& getTopology() const override
    {
        return mTopology;
    }

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