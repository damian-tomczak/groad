module;

#include <DirectXMath.h>

export module torus;
import std.core;

using namespace DirectX;

export class Torus
{
public:
    Torus(float majorRadius, float minorRadius, int majorSegments, int minorSegments)
        : mMajorRadius(majorRadius), mMinorRadius(minorRadius), mMajorSegments(majorSegments),
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

    const std::vector<float>& getGeometry()
    {
        //if (mIsDataChanged)
        //{
        //    generateGeometry();
        //}

        return mGeometry;
    }

    const std::vector<unsigned>& getTopology()
    {
        //if (mIsDataChanged)
        //{
        //    generateTopology();
        //}

        return mTopology;
    }

    //void setMajorRadius(float majorRadius)
    //{
    //    mMajorRadius = majorRadius;
    //    mIsDataChanged = true;
    //}

    //void setMinorRadius(float minorRadius)
    //{
    //    mMinorRadius = minorRadius;
    //    mIsDataChanged = true;
    //}

    //void setMajorSegments(int majorSegments)
    //{
    //    mMajorSegments = majorSegments;
    //    mIsDataChanged = true;
    //}

    //void setMinorSegments(float minorSegments)
    //{
    //    mMinorSegments = minorSegments;
    //    mIsDataChanged = true;
    //}

    void generateGeometry()
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

    void generateTopology()
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

private:
    std::vector<float> mGeometry;
    std::vector<unsigned> mTopology;
};