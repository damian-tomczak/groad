// File contains code that exists solely due to constraints listed in university assignments.

#pragma once

#include <DirectXMath.h>
#include <tuple>
#include <numbers>

namespace mg
{
using namespace DirectX;

inline std::tuple<float, float, float> getPitchYawRollFromRotationMat(XMMATRIX _rotationMat)
{
    XMFLOAT4X4 rotationMat;
    XMStoreFloat4x4(&rotationMat, _rotationMat);

    float pitch{}, yaw{}, roll{};

    const float r00 = rotationMat.m[0][0];
    const float r01 = rotationMat.m[0][1];
    const float r02 = rotationMat.m[0][2];
    const float r10 = rotationMat.m[1][0];
    const float r11 = rotationMat.m[1][1];
    const float r12 = rotationMat.m[1][2];
    const float r22 = rotationMat.m[2][2];

    if (r02 < 1.0f)
    {
        if (r02 > -1.0f)
        {
            yaw = static_cast<float>(asin(r02));
            pitch = static_cast<float>(atan2(-r12, r22));
            roll = static_cast<float>(atan2(-r01, r00));
        }
        else // r02 == -1
        {
            // Not a unique solution: roll - pitch = atan2(r10, r11)
            yaw = -std::numbers::pi_v<float> / 2.0f;
            pitch = static_cast<float>(-atan2(r10, r11));
            roll = 0.0f;
        }
    }
    else // r02 == +1
    {
        // Not a unique solution: roll + pitch = atan2(r10, r11)
        yaw = std::numbers::pi_v<float> / 2.0f;
        pitch = static_cast<float>(atan2(r10, r11));
        roll = 0.0f;
    }

    return {-pitch, -yaw, -roll};
}

inline XMMATRIX XMMatrixPerspectiveFovLH(float fovY, float aspectRatio, float nearZ, float farZ)
{
    const float yScale = 1.0f / static_cast<float>(tan(fovY / 2.0f));
    const float xScale = yScale / aspectRatio;

    return
    {
        xScale, 0.0f,   0.0f,                           0.0f,
        0.0f,   yScale, 0.0f,                           0.0f,
        0.0f,   0.0f,   farZ / (farZ - nearZ),          1.0f,
        0.0f,   0.0f,   -nearZ * farZ / (farZ - nearZ), 0.0f,
    };
}

inline XMMATRIX XMMatrixLookAtLH(const XMVECTOR& eyePosition, const XMVECTOR& focusPosition,
                                 const XMVECTOR& upDirection)
{
    XMVECTOR zAxis = XMVector3Normalize(XMVectorSubtract(focusPosition, eyePosition));

    XMVECTOR xAxis = XMVector3Normalize(XMVector3Cross(upDirection, zAxis));

    XMVECTOR yAxis = XMVector3Cross(zAxis, xAxis);

    XMVECTOR eyeDotX = -XMVector3Dot(eyePosition, xAxis);
    XMVECTOR eyeDotY = -XMVector3Dot(eyePosition, yAxis);
    XMVECTOR eyeDotZ = -XMVector3Dot(eyePosition, zAxis);

    XMMATRIX viewMatrix
    {
        XMVectorSetByIndex(xAxis, XMVectorGetX(eyeDotX), 3),
        XMVectorSetByIndex(yAxis, XMVectorGetX(eyeDotY), 3),
        XMVectorSetByIndex(zAxis, XMVectorGetX(eyeDotZ), 3),
        XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f)
    };

    return XMMatrixTranspose(viewMatrix);
}

inline bool rayIntersectsSphere(const XMVECTOR& rayOrigin, const XMVECTOR& rayDir,
                                const XMVECTOR& sphereCenter, float sphereRadius)
{
    XMVECTOR oc = XMVectorSubtract(sphereCenter, rayOrigin);

    float a = XMVectorGetX(XMVector3Dot(rayDir, rayDir));
    float b = 2.0f * XMVectorGetX(XMVector3Dot(oc, rayDir));
    float c = XMVectorGetX(XMVector3Dot(oc, oc)) - sphereRadius * sphereRadius;

    float discriminant = b * b - 4 * a * c;

    return discriminant >= 0;
}

inline XMMATRIX XMMatrixScaling(float sx, float sy, float sz)
{
    return
    {
        sx, 0.0f, 0.0f, 0.0f,
        0.0f, sy, 0.0f, 0.0f,
        0.0f, 0.0f, sz, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
}
}

#define CUSTOM_MATH

#ifdef CUSTOM_MATH
#define XMMatrixPerspectiveFovLH mg::XMMatrixPerspectiveFovLH
#define XMMatrixLookAtLH mg::XMMatrixLookAtLH
#define XMMatrixScaling mg::XMMatrixScaling
#endif