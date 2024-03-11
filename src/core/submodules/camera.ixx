module;

#include "utils.h"
#include "mg.hpp"

#include <DirectXMath.h>

export module core.camera;

using namespace DirectX;

export class Camera
{
    inline static XMVECTOR mUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    inline static XMVECTOR mRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    XMVECTOR mPosition;
    XMVECTOR mFront;

    float mYaw{};
    float mPitch{};

    float mMovementSpeed = 100.0f;
    float mMouseSensitivity = 0.01f;
    float mZoom = 45.0f;

public:
    enum CameraMovement
    {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    Camera(XMVECTOR mPosition = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), float yaw = 90.0f, float pitch = 0.0f)
        : mPosition{mPosition}, mYaw{yaw}, mPitch{pitch}
    {
        mFront = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        updateCameraVectors();
    }

    Camera(float posX, float posY, float posZ, float yaw = 90.0f, float pitch = 0.0f)
        : Camera(XMVectorSet(posX, posY, posZ, 0.0f), yaw, pitch)
    {
    }

    XMMATRIX getViewMatrix() const
    {
        return mg::XMMatrixLookAtLH(mPosition, XMVectorAdd(mPosition, mFront), mUp);
    }

    void processKeyboard(const CameraMovement direction, const float deltaTime)
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

    void processMouseMovement(float xoffset, float yoffset, bool constrainmPitch = true)
    {
        xoffset *= mMouseSensitivity;
        yoffset *= mMouseSensitivity;

        mYaw += xoffset;
        mPitch += yoffset;

        if (constrainmPitch)
        {
            mPitch = std::min(std::max(mPitch, -89.0f), 89.0f);
        }

        updateCameraVectors();
    }

    void processMouseScroll(float yoffset)
    {
        mZoom = std::min(std::max(mZoom - yoffset, 1.0f), 45.0f);
    }

private:
    void updateCameraVectors()
    {
        float yawRadians = XMConvertToRadians(mYaw);
        float pitchRadians = XMConvertToRadians(mPitch);

        XMVECTOR front;
        front = XMVectorSet(cosf(yawRadians) * cosf(pitchRadians), sinf(pitchRadians),
                            sinf(yawRadians) * cosf(pitchRadians), 0.0f
        );
        mFront = XMVector3Normalize(front);

        mRight = XMVector3Normalize(XMVector3Cross(mFront, mUp));
        mUp = XMVector3Normalize(XMVector3Cross(mRight, mFront));
    }
};
