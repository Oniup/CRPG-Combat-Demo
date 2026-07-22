#include "engine/core/Window.h"

#include "engine/core/Error.h"
#include "raylib.h"
#include <assert.h>
#include <cstdlib>
#include <limits>
#include <tuple>

namespace eng {

Window::Window(const std::string_view& title, int limitFps, WindowResolution resolution,
               WindowOptionsFlags options)
    : m_Title(title),
      m_Resolution(resolution),
      m_Options(options)
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags((int)m_Options);
    InitWindow(0, 0, m_Title.c_str());
    ASSERT(IsWindowReady(), "Failed to initialize Raylib's window for some reason");
    SetResolution(m_Resolution);

    SetExitKey(NULL);
    SetTargetFPS(limitFps);
}

Window::~Window()
{
    CloseWindow();
}

std::tuple<int, int> Window::GetResolutionSize(WindowResolution resolution)
{
    switch (resolution)
    {
    case WindowResolution::NHD:     return std::make_tuple(640, 360);
    case WindowResolution::HD:      return std::make_tuple(1280, 720);
    case WindowResolution::FullHD:  return std::make_tuple(1920, 1080);
    case WindowResolution::QHD:     return std::make_tuple(2560, 1440);
    case WindowResolution::QHDPlus: return std::make_tuple(3200, 1800);
    case WindowResolution::UHD:     return std::make_tuple(3840, 2160);
    case WindowResolution::UHD5K:   return std::make_tuple(5120, 2880);
    case WindowResolution::UHD8K:   return std::make_tuple(7680, 4320);
    default:                        return std::make_tuple(0, 0);
    }
}

WindowResolution Window::GetClosestResolutionSize(int width)
{
    WindowResolution closest = WindowResolution::Auto;
    int closestWidth         = std::numeric_limits<int>::max();

    for (size_t i = 0; i < (size_t)WindowResolution::MaxCount; i++)
    {
        int resolutionWidth = std::get<0>(Window::GetResolutionSize((WindowResolution)i));

        int diff        = std::abs(width - resolutionWidth);
        int currentDiff = std::abs(width - closestWidth);
        if (diff < currentDiff)
        {
            closestWidth = resolutionWidth;
            closest      = (WindowResolution)i;
        }
    }

    return closest;
}

int Window::GetWidth() const
{
    return GetScreenWidth();
}

int Window::GetHeight() const
{
    return GetScreenHeight();
}

bool Window::IsOpen() const
{
    return !WindowShouldClose();
}

const std::string& Window::GetTitle() const
{
    return m_Title;
}

WindowResolution Window::GetResolution() const
{
    return m_Resolution;
}

WindowOptionsFlags Window::GetOptions() const
{
    return m_Options;
}

void Window::SetTitle(const std::string_view& title)
{
    if (!title.empty())
    {
        m_Title = title;
        SetWindowTitle(m_Title.data());
    }
}

void Window::SetResolution(WindowResolution resolution, bool center)
{
    int monitor      = GetCurrentMonitor();
    int monitorWidth = GetMonitorWidth(monitor);

    if (resolution == WindowResolution::Auto)
        resolution = Window::GetClosestResolutionSize(monitorWidth - monitorWidth / 3);

    auto [width, height] = Window::GetResolutionSize(resolution);
    ASSERT(width < monitorWidth,
           "Cannot set resolution size ({}) that is bigger than the main monitor",
           (int)resolution);

    m_Resolution = resolution;
    SetWindowSize(width, height);
    if (center)
    {
        int monitorHeight = GetMonitorHeight(monitor);
        SetWindowPosition(monitorWidth / 2 - width / 2, monitorHeight / 2 - height / 2);
    }
}

} // namespace eng
