// File contains code that exists solely due to constraints listed in university assignments.

#pragma once

#include <DirectXMath.h>

namespace mg
{
inline DirectX::XMMATRIX createPerspectiveFovLH(float fovY, float aspectRatio, float nearZ, float farZ)
{
    const float yScale = 1.0f / std::tan(fovY / 2.0f);
    const float xScale = yScale / aspectRatio;

    const DirectX::XMMATRIX result
    {
        xScale, 0.0f,   0.0f,                           0.0f,
        0.0f,   yScale, 0.0f,                           0.0f,
        0.0f,   0.0f,   farZ / (farZ - nearZ),          1.0f,
        0.0f,   0.0f,   -nearZ * farZ / (farZ - nearZ), 0.0f
    };

    return result;
}

inline DirectX::XMMATRIX XMMatrixLookAtLH(const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& target,
                                   const DirectX::XMVECTOR& up)
{
    DirectX::XMVECTOR zAxis = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(target, position));
    DirectX::XMVECTOR xAxis = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, zAxis));
    DirectX::XMVECTOR yAxis = DirectX::XMVector3Cross(zAxis, xAxis);

    DirectX::XMVECTOR negPos = DirectX::XMVectorNegate(position);
    float tx = DirectX::XMVectorGetX(DirectX::XMVector3Dot(xAxis, negPos));
    float ty = DirectX::XMVectorGetY(DirectX::XMVector3Dot(yAxis, negPos));
    float tz = DirectX::XMVectorGetZ(DirectX::XMVector3Dot(zAxis, negPos));

    const DirectX::XMMATRIX viewMatrix
    {
        DirectX::XMVectorGetX(xAxis), DirectX::XMVectorGetY(xAxis), DirectX::XMVectorGetZ(xAxis), 0,
        DirectX::XMVectorGetX(yAxis), DirectX::XMVectorGetY(yAxis), DirectX::XMVectorGetZ(yAxis), 0,
        DirectX::XMVectorGetX(zAxis), DirectX::XMVectorGetY(zAxis), DirectX::XMVectorGetZ(zAxis), 0, tx, ty, tz, 1,
    };

    return viewMatrix;
}
}