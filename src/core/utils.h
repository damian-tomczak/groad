#pragma once

#include <mutex>

#ifdef ERROR
#undef ERROR
#endif

#ifdef ERR
#undef ERR
#endif

#ifdef _WIN32
#define FUNCTION_SIGNATURE __FUNCSIG__
#else
#error not implemented
#endif

#ifndef NDEBUG
#define DEBUG_LOG_INFO std::string("(" __FILE__ ":" STR(__LINE__) " " FUNCTION_SIGNATURE ") ") +
#else
#define DEBUG_LOG_INFO
#endif

#define STR(x) XSTR(x)
#define XSTR(x) #x

using namespace std::literals;

#define LOG(msg) log(DEBUG_LOG_INFO msg, LogLevel::INFO);
#define WARN(msg) log(DEBUG_LOG_INFO msg, LogLevel::WARNING);
#define ERR(msg) log(DEBUG_LOG_INFO msg, LogLevel::ERROR);

// clang-format off
#define ASSERT(x)                   \
    if (!(x))                       \
    {                               \
        ERR("Assert occured: " #x); \
    }

#define HR(x)                                                                   \
    if (const auto hr = x; FAILED(hr))                                          \
    {                                                                           \
        ERR(std::format(#x " {}({})", std::system_category().message(hr), hr)); \
    }


#define SINGLETON_BODY(ClassName)   \
    NOT_COPYABLE(ClassName);        \
    private: ClassName() = default; \
    friend Singleton<ClassName>;
// clang-format on

#ifdef WIN32
#elif __GNUC__
#define __forceinline __attribute__((always_inline))
#else
#error "not implemented"
#endif // !__GNUC__

enum class LogLevel : uint8_t
{
    DEFAULT,
    INFO = DEFAULT,
    WARNING,
    ERROR,
    COUNT
};

void log(const std::string_view msg, const LogLevel level = LogLevel::DEFAULT);

class NonCopyable
{
protected:
    NonCopyable() = default;

public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

class NonMoveable
{
protected:
    NonMoveable() = default;

public:
    NonMoveable(NonMoveable&&) = delete;
    NonMoveable& operator=(NonMoveable&&) = delete;
};

class NonCopyableAndMoveable : public NonCopyable, public NonMoveable
{
};

template <typename DerivedClass>
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

        ASSERT(ptr != nullptr);

        return static_cast<DerivedClass&>(*ptr);
    }

    static void destroyInstance()
    {
        pInstance.reset();
    }

protected:
    Singleton() = default;

private:
    inline static std::mutex mutex;
    inline static std::atomic<Singleton*> pInstance;
};
