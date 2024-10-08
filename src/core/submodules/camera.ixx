module;

#include "utils.h"
#include "mg.hpp"

export module core.camera;
import std.core;

export inline constexpr float mouseSensitivity = 0.05f;

export class Camera : public NonCopyableAndMoveable
{
    inline static XMVECTOR mUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    inline static XMVECTOR mRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    XMVECTOR mPos;
    XMVECTOR mFront;

    float mYaw{};
    float mPitch{};

    float mMovementSpeed = 25.0f;
    float mZoom = 45.0f;

public:
    enum CameraMovement
    {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    Camera(XMVECTOR mPos, float yaw, float pitch)
        : mPos{mPos}, mYaw{yaw}, mPitch{pitch}
    {
        updateCameraVectors();
    }

    Camera(float posX, float posY, float posZ, float yaw = 90.0f, float pitch = 0.0f)
        : Camera(XMVectorSet(posX, posY, posZ, 0.0f), yaw, pitch)
    {
    }

    XMMATRIX getViewMatrix() const
    {
        return XMMatrixLookAtRH(mPos, XMVectorAdd(mPos, mFront), mUp);
    }

    void moveCamera(const CameraMovement dir, float dt)
    {
        updateCameraVectors();

        const float velocity = mMovementSpeed * dt;

        switch (dir)
        {
        case FORWARD:
            mPos -= XMVectorScale(mFront, velocity);
            break;
        case BACKWARD:
            mPos += XMVectorScale(mFront, velocity);
            break;
        case LEFT:
            mPos -= XMVectorScale(mRight, velocity);
            break;
        case RIGHT:
            mPos += XMVectorScale(mRight, velocity);
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

    [[nodiscard]] XMVECTOR getFront() const
    {
        return mFront;
    }

    [[nodiscard]] XMVECTOR getRight() const
    {
        return mRight;
    }

    [[nodiscard]] XMVECTOR getUp() const
    {
        return mUp;
    }

    [[nodiscard]] XMVECTOR getPos() const
    {
        return mPos;
    }

    XMMATRIX mViewMtx = XMMatrixIdentity();
    XMMATRIX mProjMtx = XMMatrixIdentity();

private:
    void updateCameraVectors()
    {
        float yawRadians = XMConvertToRadians(mYaw);
        float pitchRadians = XMConvertToRadians(mPitch);

        XMVECTOR front = XMVectorSet(cosf(yawRadians) * cosf(pitchRadians), sinf(pitchRadians), sinf(yawRadians) * cosf(pitchRadians), 0.0f);
        mFront = XMVector3Normalize(front);
    }
};
