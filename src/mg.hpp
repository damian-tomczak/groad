// File contains code that exists solely due to constraints listed in university assignments.

#pragma once

#include <DirectXMath.h>
#include <tuple>
#include <numbers>

#include "Serializer.h"

//#define CUSTOM_MATH

#ifdef CUSTOM_MATH
using DirectX::FXMVECTOR;
using DirectX::XMFLOAT4X4;
using DirectX::XMMATRIX;
using DirectX::XMVECTOR;
using DirectX::XMVectorGetW;
using DirectX::XMVectorGetX;
using DirectX::XMVectorGetY;
using DirectX::XMVectorGetZ;
using DirectX::XMVectorReplicate;
using DirectX::XMVectorSet;
using DirectX::XMVectorSubtract;
#else
using namespace DirectX;
#endif

namespace mg
{
bool rayIntersectsSphere(FXMVECTOR rayOrigin, FXMVECTOR rayDir, FXMVECTOR sphereCenter,
                         float sphereRadius);
std::tuple<float, float, float> getPitchYawRollFromRotationMat(XMMATRIX _rotationMat);

XMMATRIX XMMatrixPerspectiveFovLH(float fovY, float aspectRatio, float nearZ, float farZ);
XMMATRIX XMMatrixLookAtLH(FXMVECTOR eyePos, FXMVECTOR focusPos, FXMVECTOR upDir);
XMMATRIX XMMatrixLookAtRH(FXMVECTOR eyePos, FXMVECTOR focusPos, FXMVECTOR upDir);
XMMATRIX XMMatrixScaling(float sx, float sy, float sz);
XMMATRIX XMMatrixRotationX(float angleRadians);
XMMATRIX XMMatrixRotationY(float angleRadians);
XMMATRIX XMMatrixRotationZ(float angleRadians);
XMVECTOR XMVectorScale(FXMVECTOR v, float scale);
XMVECTOR XMVector3Normalize(FXMVECTOR v);
XMVECTOR XMVector3Dot(FXMVECTOR v1, FXMVECTOR v2);
XMVECTOR XMVector3Cross(FXMVECTOR v1, FXMVECTOR v2);
}

#ifdef CUSTOM_MATH
#define XMMatrixPerspectiveFovLH mg::XMMatrixPerspectiveFovLH
#define XMMatrixLookAtLH         mg::XMMatrixLookAtLH
#define XMMatrixLookAtRH         mg::XMMatrixLookAtRH
#define XMMatrixScaling          mg::XMMatrixScaling
#define XMMatrixRotationX        mg::XMMatrixRotationX
#define XMMatrixRotationY        mg::XMMatrixRotationY
#define XMMatrixRotationZ        mg::XMMatrixRotationZ
#define XMVectorScale            mg::XMVectorScale
#define XMVector3Normalize       mg::XMVector3Normalize
#define XMVector3Dot             mg::XMVector3Dot
#define XMVector3Cross           mg::XMVector3Cross
#else
#define XMMatrixPerspectiveFovLH DirectX::XMMatrixPerspectiveFovLH
#define XMMatrixLookAtLH         DirectX::XMMatrixLookAtLH
#define XMMatrixLookAtRH         DirectX::XMMatrixLookAtRH
#define XMMatrixScaling          DirectX::XMMatrixScaling
#define XMMatrixRotationX        DirectX::XMMatrixRotationX
#define XMMatrixRotationY        DirectX::XMMatrixRotationY
#define XMMatrixRotationZ        DirectX::XMMatrixRotationZ
#define XMVectorScale            DirectX::XMVectorScale
#define XMVector3Normalize       DirectX::XMVector3Normalize
#define XMVector3Dot             DirectX::XMVector3Dot
#define XMVector3Cross           DirectX::XMVector3Cross
#endif

inline bool mg::rayIntersectsSphere(FXMVECTOR rayOrigin, FXMVECTOR rayDir, FXMVECTOR sphereCenter,
                                float sphereRadius)
{
    XMVECTOR oc = XMVectorSubtract(sphereCenter, rayOrigin);

    float a = XMVectorGetX(XMVector3Dot(rayDir, rayDir));
    float b = 2.0f * XMVectorGetX(XMVector3Dot(oc, rayDir));
    float c = XMVectorGetX(XMVector3Dot(oc, oc)) - sphereRadius * sphereRadius;

    float discriminant = b * b - 4 * a * c;

    return discriminant >= 0;
}

inline std::tuple<float, float, float> mg::getPitchYawRollFromRotationMat(XMMATRIX _rotationMat)
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

#ifdef CUSTOM_MATH
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

inline XMMATRIX XMMatrixLookAtLH(FXMVECTOR eyePos, FXMVECTOR focusPos, FXMVECTOR upDir)
{
    XMVECTOR zAxis = XMVector3Normalize(XMVectorSubtract(focusPos, eyePos));
    XMVECTOR xAxis = XMVector3Normalize(XMVector3Cross(upDir, zAxis));
    XMVECTOR yAxis = XMVector3Cross(zAxis, xAxis);

    float tx = -XMVectorGetX(XMVector3Dot(xAxis, eyePos));
    float ty = -XMVectorGetY(XMVector3Dot(yAxis, eyePos));
    float tz = -XMVectorGetZ(XMVector3Dot(zAxis, eyePos));

    return
    {
        XMVectorSet(XMVectorGetX(xAxis), XMVectorGetX(yAxis), XMVectorGetX(zAxis), 0.0f),
        XMVectorSet(XMVectorGetY(xAxis), XMVectorGetY(yAxis), XMVectorGetY(zAxis), 0.0f),
        XMVectorSet(XMVectorGetZ(xAxis), XMVectorGetZ(yAxis), XMVectorGetZ(zAxis), 0.0f),
        XMVectorSet(tx,                  ty,                  tz,                  1.0f)
    };
}

inline XMMATRIX XMMatrixLookAtRH(FXMVECTOR eyePos, FXMVECTOR focusPos, FXMVECTOR upDir)
{
    XMVECTOR zAxis = XMVector3Normalize(XMVectorSubtract(eyePos, focusPos));
    XMVECTOR xAxis = XMVector3Normalize(XMVector3Cross(upDir, zAxis));
    XMVECTOR yAxis = XMVector3Cross(zAxis, xAxis);

    float tx = -XMVectorGetX(XMVector3Dot(xAxis, eyePos));
    float ty = -XMVectorGetY(XMVector3Dot(yAxis, eyePos));
    float tz = -XMVectorGetZ(XMVector3Dot(zAxis, eyePos));

    return
    {
        XMVectorSet(XMVectorGetX(xAxis), XMVectorGetX(yAxis), XMVectorGetX(zAxis), 0.0f),
        XMVectorSet(XMVectorGetY(xAxis), XMVectorGetY(yAxis), XMVectorGetY(zAxis), 0.0f),
        XMVectorSet(XMVectorGetZ(xAxis), XMVectorGetZ(yAxis), XMVectorGetZ(zAxis), 0.0f),
        XMVectorSet(tx,                  ty,                  tz,                  1.0f)
    };
}

inline XMMATRIX XMMatrixScaling(float sx, float sy, float sz)
{
    return
    {
        sx, 0.0f,   0.0f, 0.0f,
        0.0f, sy,   0.0f, 0.0f,
        0.0f, 0.0f, sz,   0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
}

inline XMMATRIX XMMatrixRotationX(float angleRadians)
{
    const float cosTheta = static_cast<float>(cos(angleRadians));
    const float sinTheta = static_cast<float>(sin(angleRadians));

    return
    {
        1.0f, 0.0f,      0.0f,     0.0f,
        0.0f, cosTheta,  sinTheta, 0.0f,
        0.0f, -sinTheta, cosTheta, 0.0f,
        0.0f, 0.0f,      0.0f,     1.0f,
    };
}

inline XMMATRIX XMMatrixRotationY(float angleRadians)
{
    float cosTheta = static_cast<float>(cos(angleRadians));
    float sinTheta = static_cast<float>(sin(angleRadians));

    return
    {
        cosTheta, 0.0f, -sinTheta, 0.0f,
        0.0f,     1.0f, 0.0f,      0.0f,
        sinTheta, 0.0f, cosTheta,  0.0f,
        0.0f,     0.0f, 0.0f,      1.0f,
    };
}

inline XMMATRIX XMMatrixRotationZ(float angleRadians)
{
    float cosTheta = static_cast<float>(cos(angleRadians));
    float sinTheta = static_cast<float>(sin(angleRadians));

    return
    {
        cosTheta,  sinTheta, 0.0f, 0.0f,
        -sinTheta, cosTheta, 0.0f, 0.0f,
        0.0f,      0.0f,     1.0f, 0.0f,
        0.0f,      0.0f,     0.0f, 1.0f,
    };
}

inline XMVECTOR XMVectorScale(FXMVECTOR v, float scale)
{
    return {XMVectorGetX(v) * scale, XMVectorGetY(v) * scale, XMVectorGetZ(v) * scale, XMVectorGetW(v) * scale};
}

inline XMVECTOR XMVector3Normalize(FXMVECTOR v)
{
    const float magSquare =
        XMVectorGetX(v) * XMVectorGetX(v) + XMVectorGetY(v) * XMVectorGetY(v) + XMVectorGetZ(v) * XMVectorGetZ(v);

    const float mag = sqrtf(magSquare);

    if (mag > 0.f)
    {
        return {XMVectorGetX(v) / mag, XMVectorGetY(v) / mag, XMVectorGetZ(v) / mag, XMVectorGetW(v) / mag};
    }

    return v;
}

inline XMVECTOR XMVector3Dot(FXMVECTOR v1, FXMVECTOR v2)
{
    float x1 = XMVectorGetX(v1);
    float y1 = XMVectorGetY(v1);
    float z1 = XMVectorGetZ(v1);

    float x2 = XMVectorGetX(v2);
    float y2 = XMVectorGetY(v2);
    float z2 = XMVectorGetZ(v2);

    float dotProduct = x1 * x2 + y1 * y2 + z1 * z2;

    return XMVectorReplicate(dotProduct);
}

inline XMVECTOR XMVector3Cross(FXMVECTOR v1, FXMVECTOR v2)
{
    float x1 = XMVectorGetX(v1), y1 = XMVectorGetY(v1), z1 = XMVectorGetZ(v1);
    float x2 = XMVectorGetX(v2), y2 = XMVectorGetY(v2), z2 = XMVectorGetZ(v2);

    float cx = y1 * z2 - z1 * y2;
    float cy = z1 * x2 - x1 * z2;
    float cz = x1 * y2 - y1 * x2;

    return {cx, cy, cz, 0};
}
#endif
