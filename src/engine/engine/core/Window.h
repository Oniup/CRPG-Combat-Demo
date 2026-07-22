#pragma once

#include <string>
#include <tuple>

namespace eng {

enum class WindowResolution : size_t
{
    Auto,
    NHD,
    HD,
    FullHD,
    QHD,
    QHDPlus,
    UHD,
    UHD5K,
    UHD8K,
    MaxCount,
};

enum WindowOptionsFlags
{
    WindowOption_None             = 0X00000000,
    WindowOption_VsycInt          = 0X00000040,
    WindowOption_FullscreenMode   = 0X00000002,
    WindowOption_BorderlessMode   = 0X00008000,
    WindowOption_ManualResizable  = 0X00000004,
    WindowOption_Undecorated      = 0X00000008,
    WindowOption_Hidden           = 0X00000080,
    WindowOption_Minimized        = 0X00000200,
    WindowOption_Maximized        = 0X00000400,
    WindowOption_UnFocused        = 0X00000800,
    WindowOption_TopMost          = 0X00001000,
    WindowOption_AlwaysRun        = 0X00000100,
    WindowOption_Transparent      = 0X00000010,
    WindowOption_HighDpi          = 0X00002000,
    WindowOption_MousePassThrough = 0X00004000,
    WindowOption_Msaa4xHint       = 0X00000020,
    WindowOption_InterlacedHint   = 0X00010000,
};

class Window
{

public:
    Window(const std::string_view& title, int limitFps, WindowResolution resolution,
           WindowOptionsFlags options);
    ~Window();

    static std::tuple<int, int> GetResolutionSize(WindowResolution resolution);
    static WindowResolution GetClosestResolutionSize(int width);

    int GetWidth() const;
    int GetHeight() const;

    bool IsOpen() const;

    const std::string& GetTitle() const;
    WindowResolution GetResolution() const;
    WindowOptionsFlags GetOptions() const;

    void SetTitle(const std::string_view& title);
    void SetResolution(WindowResolution resolution, bool center = true);

private:
    std::string m_Title;
    WindowResolution m_Resolution;
    WindowOptionsFlags m_Options;
};

} // namespace eng
