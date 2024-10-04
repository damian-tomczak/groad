#ifdef _WIN32
#include <Windows.h>
#endif

#include "utils.h"

#include <sstream>

import std.core;

namespace
{
std::mutex ioMutex;
}

enum class Color
{
    DEFAULT,
    GREEN,
    YELLOW,
    RED,
    COUNT
};

inline struct ColorEnd
{
} colorEnd{};

std::ostream& operator<<(std::ostream& ostream, ColorEnd)
{
    ostream << "\033[0m";
    return ostream;
}

std::wostream& operator<<(std::wostream& ostream, ColorEnd)
{
    ostream << L"\033[0m";
    return ostream;
}

std::ostream& operator<<(std::ostream& ostream, const Color color)
{
    switch (color)
    {
    case Color::GREEN:
        ostream << "\033[32m";
        break;
    case Color::YELLOW:
        ostream << "\033[33m";
        break;
    case Color::RED:
        ostream << "\033[91m";
        break;
    default:
        ostream << "COLOR_PARSING_UNKNOWN";
        break;
    }

    return ostream;
}

std::wostream& operator<<(std::wostream& wostream, const Color color)
{
    switch (color)
    {
    case Color::GREEN:
        wostream << L"\033[32m";
        break;
    case Color::YELLOW:
        wostream << L"\033[33m";
        break;
    case Color::RED:
        wostream << L"\033[91m";
        break;
    default:
        wostream << L"COLOR_PARSING_UNKNOWN";
        break;
    }

    return wostream;
}

constexpr const char* logLevelToStr(const LogLevel level);
constexpr const wchar_t* logLevelToWStr(const LogLevel level);
constexpr Color logLevelToColor(const LogLevel level);

void log(std::string_view msg, const LogLevel level, bool shouldTerminate)
{
    const auto now = std::chrono::system_clock::now();
    const time_t nowTime = std::chrono::system_clock::to_time_t(now);

    tm nowTm{};
#ifdef _WIN32
    localtime_s(&nowTm, &nowTime);
#else
#error "not implemented"
#endif

    const auto secondRemainder = now - std::chrono::system_clock::from_time_t(nowTime);
    const int64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(secondRemainder).count();

    std::ostringstream out;
    out.fill('0');
    out << logLevelToColor(level) << "[" << std::setw(2) << nowTm.tm_hour << ":" << std::setw(2) << nowTm.tm_min << ":"
        << std::setw(2) << nowTm.tm_sec << "." << std::setw(3) << milliseconds << "]"
        << "[" << logLevelToStr(level) << "] " << msg << colorEnd << std::endl;

    std::lock_guard lock{ioMutex};
    if (level == LogLevel::ERROR)
    {
        std::clog << out.str();

#ifdef DEBUG
#ifdef _WIN32
        DebugBreak();
#else
#error not implemented
#endif // _WIN32
#endif // !NDEBUG
    }
    else
    {
        std::cout << out.str();
    }

    if (shouldTerminate && (level == LogLevel::ERROR))
    {
        std::terminate();
    }
}

void log(const std::wstring_view msg, const LogLevel level, bool shouldTerminate)
{
    const auto now = std::chrono::system_clock::now();
    const time_t nowTime = std::chrono::system_clock::to_time_t(now);

    tm nowTm{};
#ifdef _WIN32
    localtime_s(&nowTm, &nowTime);
#else
#error "not implemented"
#endif

    const auto secondRemainder = now - std::chrono::system_clock::from_time_t(nowTime);
    const int64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(secondRemainder).count();

    std::wostringstream out;
    out.fill('0');
    out << logLevelToColor(level) << "[" << std::setw(2) << nowTm.tm_hour << ":" << std::setw(2) << nowTm.tm_min << ":"
        << std::setw(2) << nowTm.tm_sec << "." << std::setw(3) << milliseconds << "]"
        << "[" << logLevelToWStr(level) << "] " << msg << colorEnd << std::endl;

    std::lock_guard lock{ioMutex};
    if (level == LogLevel::ERROR)
    {
        std::wclog << out.str();

#ifdef DEBUG
#ifdef _WIN32
        DebugBreak();
#else
#error not implemented
#endif // _WIN32
#endif // !NDEBUG
    }
    else
    {
        std::wcout << out.str();
    }

    if (shouldTerminate && (level == LogLevel::ERROR))
    {
        std::terminate();
    }
}

constexpr const char* logLevelToStr(const LogLevel level)
{
    switch (level)
    {
    case LogLevel::INFO:
        return "INFO   ";
    case LogLevel::WARNING:
        return "WARNING";
    case LogLevel::ERROR:
        return "ERROR  ";
    }

    return "UNKNOWN";
}

constexpr const wchar_t* logLevelToWStr(const LogLevel level)
{
    switch (level)
    {
    case LogLevel::INFO:
        return L"INFO   ";
    case LogLevel::WARNING:
        return L"WARNING";
    case LogLevel::ERROR:
        return L"ERROR  ";
    }

    return L"UNKNOWN";
}

constexpr Color logLevelToColor(const LogLevel level)
{
    switch (level)
    {
    case LogLevel::INFO:
        return Color::GREEN;
    case LogLevel::WARNING:
        return Color::YELLOW;
    case LogLevel::ERROR:
        return Color::RED;
    }

    return {};
}