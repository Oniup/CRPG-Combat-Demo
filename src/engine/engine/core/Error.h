#pragma once

#include "fmt/core.h"
#include "fmt/std.h"
#include <cstdio>
#include <string_view>

namespace eng::err::intl {

enum SeverityColor
{
    Severity_Warning,
    Severity_Error,
};

void PrintErrorHeaderMessage(SeverityColor severity, std::string_view file,
                             std::string_view function, int line, std::string_view expression);

void PrintAssertErrorMessage(SeverityColor severity, std::string_view file,
                             std::string_view function, int line);

void OnAssert(std::string_view file, std::string_view function, int line,
              std::string_view expression);

template <typename... Args>
void OnAssert(std::string_view file, std::string_view function, int line,
              std::string_view expression, fmt::format_string<Args...> format, Args&&... args)
{
    PrintErrorHeaderMessage(Severity_Error, file, function, line, expression);

    constexpr int maxBufferSize = 2048;
    fmt::println(stderr, format, std::forward<Args>(args)...);
    std::fflush(stderr);
}

template <typename... Args>
void OnError(SeverityColor severity, std::string_view file, std::string_view function, int line,
             fmt::format_string<Args...> format, Args&&... args)
{
    PrintAssertErrorMessage(severity, file, function, line);

    constexpr int maxBufferSize = 2048;
    fmt::println(stderr, format, std::forward<Args>(args)...);
    std::fflush(stderr);
}

} // namespace eng::err::intl

// https://github.com/scottt/debugbreak/blob/master/debugbreak.h
#ifndef NDEBUG
#    if defined(_MSC_VER)
#        define DEBUG_BREAK __debugbreak()
#    else
// Dont know if this works only tested compiling on windows using MSVC and LLVM's Clang
#        define DEBUG_BREAK __builtin_trap()
#    endif
#else
#    define DEBUG_BREAK std::DEBUG_BREAK(-1)
#endif

#define ASSERT(expression_, ...)                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(expression_))                                                                        \
        {                                                                                          \
            eng::err::intl::OnAssert(                                                              \
                __FILE__, __FUNCTION__, __LINE__, #expression_ __VA_OPT__(, ) __VA_ARGS__);        \
            DEBUG_BREAK;                                                                           \
            std::exit(-1);                                                                         \
        }                                                                                          \
    }                                                                                              \
    while (false)

#ifndef NDEBUG
#    define DASSERT(expression_, ...) ASSERT(expression_ __VA_OPT__(, ) __VA_ARGS__)
#else
#    define DASSERT(expression_, ...)
#endif

#define FATAL(...)                                                                                 \
    do                                                                                             \
    {                                                                                              \
        eng::err::intl::OnError(                                                                   \
            eng::err::intl::Severity_Error, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);        \
        DEBUG_BREAK;                                                                               \
        std::exit(-1);                                                                             \
    }                                                                                              \
    while (false)

#define WARNING(...)                                                                               \
    eng::err::intl::OnError(                                                                       \
        eng::err::intl::Severity_Warning, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#define ERROR(...)                                                                                 \
    eng::err::intl::OnError(                                                                       \
        eng::err::intl::Severity_Error, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
