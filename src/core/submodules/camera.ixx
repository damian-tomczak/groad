module;

#include "utils.h"
#include "mg.hpp"

#include <DirectXMath.h>

export module core.camera;
import std.core;

using namespace DirectX;

export inline constexpr float mouseSensitivity = 0.05f;

export class Camera : public NonCopyableAndMoveable
{
    inline static XMVECTOR mUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    inline static XMVECTOR mRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    XMVECTOR mPosition;
    XMVECTOR mFront;

    float mYaw{};
    float mPitch{};

    float mMovementSpeed = 500.0f;
    float mZoom = 45.0f;

public:
    enum CameraMovement
    {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    Camera(XMVECTOR mPosition, float yaw, float pitch)
        : mPosition{mPosition}, mYaw{yaw}, mPitch{pitch}
    {
        updateCameraVectors();
    }

    Camera(float posX, float posY, float posZ, float yaw = 90.0f, float pitch = 0.0f)
        : Camera(XMVectorSet(posX, posY, posZ, 0.0f), yaw, pitch)
    {
    }

    XMMATRIX getViewMatrix() const
    {
        return XMMatrixLookAtLH(mPosition, XMVectorAdd(mPosition, mFront), mUp);
    }

    void moveCamera(const CameraMovement direction, float deltaTime)
    {
        const float velocity = mMovementSpeed * deltaTime;

        switch (direction)
        {
        case FORWARD:
            mPosition = XMVectorAdd(mPosition, XMVectorScale(mFront, velocity));
            break;
        case BACKWARD:
            mPosition = XMVectorSubtract(mPosition, XMVectorScale(mFront, velocity));
            break;
        case LEFT:
            mPosition = XMVectorSubtract(mPosition, XMVectorScale(mRight, velocity));
            break;
        case RIGHT:
            mPosition = XMVectorAdd(mPosition, XMVectorScale(mRight, velocity));
            break;
        default:
            ASSERT(false);
        }
    }

    void rotateCamera(float xoffset, float yoffset)
    {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        mYaw += xoffset;
        mPitch += yoffset;

        updateCameraVectors();
    }

    void setZoom(float yoffset)
    {
        mZoom = std::min(std::max(mZoom - yoffset / abs(yoffset), 1.0f), 45.0f);
    }

    [[nodiscard]] float getZoom() const
    {
        return mZoom;
    }

    XMVECTOR getLookingAtVec(float offset = 1.0f) const
    {
        return mPosition + mFront * offset;
    }

private:
    void updateCameraVectors()
    {
        float yawRadians = XMConvertToRadians(mYaw);
        float pitchRadians = XMConvertToRadians(mPitch);

        XMVECTOR front = XMVectorSet(cosf(yawRadians) * cosf(pitchRadians), sinf(pitchRadians), sinf(yawRadians) * cosf(pitchRadians), 0.0f);
        mFront = XMVector3Normalize(front);

        mRight = XMVector3Normalize(XMVector3Cross(mFront, mUp));
        mUp = XMVector3Normalize(XMVector3Cross(mRight, mFront));
    }
};
