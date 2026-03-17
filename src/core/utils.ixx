export module utils;

export import <algorithm>;
export import <array>;
export import <atomic>;
export import <bitset>;
export import <cassert>;
export import <charconv>;
export import <chrono>;
export import <cmath>;
export import <compare>;
export import <concepts>;
export import <cstdint>;
export import <deque>;
export import <exception>;
export import <filesystem>;
export import <format>;
export import <fstream>;
export import <functional>;
export import <iomanip>;
export import <iosfwd>;
export import <iostream>;
export import <iterator>;
export import <limits>;
export import <list>;
export import <locale>;
export import <map>;
export import <memory>;
export import <mutex>;
export import <numbers>;
export import <optional>;
export import <ostream>;
export import <queue>;
export import <random>;
export import <ranges>;
export import <set>;
export import <span>;
export import <sstream>;
export import <stack>;
export import <stdexcept>;
export import <string>;
export import <string_view>;
export import <system_error>;
export import <tuple>;
export import <type_traits>;
export import <unordered_map>;
export import <unordered_set>;
export import <utility>;
export import <variant>;
export import <vector>;

export import <d3d11.h>;
export import <DirectXColors.h>;
export import <DirectXMath.h>;
export import <wrl/client.h>;

export namespace fs = std::filesystem;
export namespace Colors = DirectX::Colors;

export using Microsoft::WRL::ComPtr;

export using DirectX::FXMVECTOR;
export using DirectX::XMConvertToRadians;
export using DirectX::XMFLOAT3;
export using DirectX::XMFLOAT4;
export using DirectX::XMFLOAT4X4;
export using DirectX::XMLoadFloat3;
export using DirectX::XMMATRIX;
export using DirectX::XMMatrixIdentity;
export using DirectX::XMMatrixInverse;
export using DirectX::XMMatrixLookAtLH;
export using DirectX::XMMatrixLookAtRH;
export using DirectX::XMMatrixMultiply;
export using DirectX::XMMatrixPerspectiveFovLH;
export using DirectX::XMMatrixPerspectiveFovRH;
export using DirectX::XMMatrixPerspectiveOffCenterRH;
export using DirectX::XMMatrixRotationX;
export using DirectX::XMMatrixRotationY;
export using DirectX::XMMatrixRotationZ;
export using DirectX::XMMatrixScaling;
export using DirectX::XMMatrixScalingFromVector;
export using DirectX::XMMatrixTranslation;
export using DirectX::XMMatrixTranslationFromVector;
export using DirectX::XMMatrixTranspose;
export using DirectX::XMVECTOR;
export using DirectX::XMVECTORF32;
export using DirectX::XMStoreFloat;
export using DirectX::XMStoreFloat3;
export using DirectX::XMStoreFloat4x4;
export using DirectX::XMVector3Cross;
export using DirectX::XMVector3Dot;
export using DirectX::XMVector3Length;
export using DirectX::XMVector3LengthSq;
export using DirectX::XMVector3Normalize;
export using DirectX::XMVector4Transform;
export using DirectX::XMVectorAdd;
export using DirectX::XMVectorDivide;
export using DirectX::XMVectorGetW;
export using DirectX::XMVectorGetX;
export using DirectX::XMVectorGetY;
export using DirectX::XMVectorGetZ;
export using DirectX::XMVectorReplicate;
export using DirectX::XMVectorScale;
export using DirectX::XMVectorSet;
export using DirectX::XMVectorSetW;
export using DirectX::XMVectorSubtract;
export using DirectX::XMVectorZero;

// Windows headers define ERROR as a macro; clear it before declaring LogLevel.
#ifdef ERROR
#undef ERROR
#endif

export enum class LogLevel : std::uint8_t
{
    DEFAULT,
    INFO = DEFAULT,
    WARNING,
    ERROR,
    COUNT
};

export void log(std::string_view msg, LogLevel level = LogLevel::DEFAULT, bool shouldTerminate = true);
export void log(std::wstring_view msg, LogLevel level = LogLevel::DEFAULT, bool shouldTerminate = true);

export class NonCopyable
{
protected:
    NonCopyable() = default;

public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

export class NonMoveable
{
protected:
    NonMoveable() = default;

public:
    NonMoveable(NonMoveable&&) = delete;
    NonMoveable& operator=(NonMoveable&&) = delete;
};

export class NonCopyableAndMoveable : public NonCopyable, public NonMoveable
{
};

export template <typename DerivedClass>
class Singleton : public NonCopyable
{
public:
    static DerivedClass& getInstance()
    {
        Singleton* ptr = pInstance.load(std::memory_order_acquire);
        if (ptr == nullptr)
        {
            std::lock_guard<std::mutex> lock(mutex);
            ptr = pInstance.load(std::memory_order_relaxed);
            if (ptr == nullptr)
            {
                ptr = new DerivedClass;
                pInstance.store(ptr, std::memory_order_release);
            }
        }

        return static_cast<DerivedClass&>(*ptr);
    }

    static void destroyInstance()
    {
        if (auto* ptr = pInstance.exchange(nullptr, std::memory_order_acq_rel); ptr != nullptr)
        {
            delete static_cast<DerivedClass*>(ptr);
        }
    }

protected:
    Singleton() = default;

private:
    inline static std::mutex mutex;
    inline static std::atomic<Singleton*> pInstance{};
};
