#pragma once

// Compatibility shim for the C++20 `utils` module.
// Import `utils` before including this header.

#ifdef ERROR
#undef ERROR
#endif

#ifdef ERR
#undef ERR
#endif

#ifndef FAILED
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#endif

#define STR(x) XSTR(x)
#define XSTR(x) #x

#ifdef _WIN32
#define FUNCTION_SIGNATURE __FUNCSIG__
#else
#error not implemented
#endif

#ifdef DEBUG
#define DEBUG_LOG_INFO std::string("(" __FILE__ ":" STR(__LINE__) " " FUNCTION_SIGNATURE ") ") +
#define WDEBUG_LOG_INFO std::wstring(L"(" __FILE__ ":" STR(__LINE__) " " FUNCTION_SIGNATURE ") ") +
#else
#define DEBUG_LOG_INFO
#define WDEBUG_LOG_INFO
#endif

#define LOG(msg) log(DEBUG_LOG_INFO msg, LogLevel::INFO);
#define WARN(msg) log(DEBUG_LOG_INFO msg, LogLevel::WARNING);
#define ERR(msg) log(DEBUG_LOG_INFO msg, LogLevel::ERROR);
#define ERR_NOTERMINATE(msg) log(DEBUG_LOG_INFO msg, LogLevel::ERROR, false);

#define WLOG(msg) log(WDEBUG_LOG_INFO msg, LogLevel::INFO);
#define WWARN(msg) log(WDEBUG_LOG_INFO msg, LogLevel::WARNING);
#define WERR(msg) log(WDEBUG_LOG_INFO msg, LogLevel::ERROR);
#define WERR_NOTERMINATE(msg) log(WDEBUG_LOG_INFO msg, LogLevel::ERROR, false);

#ifdef DEBUG
#define ASSERT(x)                                \
    if (!(x))                                    \
    {                                            \
        ERR_NOTERMINATE("Assert occured: " #x);  \
    }
#else
#define ASSERT
#endif

#define HR(x)                                                                   \
    if (const auto hr = x; FAILED(hr))                                          \
    {                                                                           \
        ERR(std::format(#x " {}({})", std::system_category().message(hr), hr)); \
    }
