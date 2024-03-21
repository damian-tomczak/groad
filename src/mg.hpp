// File contains code that exists solely due to constraints listed in university assignments.

#pragma once

#include <DirectXMath.h>

namespace mg
{
using namespace DirectX;

inline XMMATRIX XMMatrixPerspectiveFovLH(float fovY, float aspectRatio, float nearZ, float farZ)
{
    const float yScale = 1.0f / static_cast<float>(tan(fovY / 2.0f));
    const float xScale = yScale / aspectRatio;

    const XMMATRIX result
    {
        xScale, 0.0f,   0.0f,                           0.0f,
        0.0f,   yScale, 0.0f,                           0.0f,
        0.0f,   0.0f,   farZ / (farZ - nearZ),          1.0f,
        0.0f,   0.0f,   -nearZ * farZ / (farZ - nearZ), 0.0f
    };

    return result;
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

    XMMATRIX viewMatrix = {
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
}

#define CUSTOM_MATH

#ifdef CUSTOM_MATH
#define XMMatrixPerspectiveFovLH mg::XMMatrixPerspectiveFovLH
#define XMMatrixLookAtLH mg::XMMatrixLookAtLH
#endif