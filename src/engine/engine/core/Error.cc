#include "engine/core/Error.h"

#include "fmt/color.h"
#include "fmt/core.h"

namespace eng::err::intl {

fmt::text_style getSeverityColor(SeverityColor severity)
{
    switch (severity)
    {
    case Severity_Warning:
        return fmt::fg(fmt::color::light_golden_rod_yellow) | fmt::emphasis::bold;
    case Severity_Error: return fmt::fg(fmt::color::dark_red) | fmt::emphasis::bold;
    }
}

void PrintErrorHeaderMessage(SeverityColor severity, std::string_view file,
                             std::string_view function, int line, std::string_view expression)
{
    fmt::print(stderr,
               getSeverityColor(severity),
               "in '{}' at {}:{} --> '{}'\n",
               file,
               function,
               line,
               expression);
}

void PrintAssertErrorMessage(SeverityColor severity, std::string_view file,
                             std::string_view function, int line)
{
    fmt::print(stderr, getSeverityColor(severity), "in '{}' at {}:{}\n", file, function, line);
}

void OnAssert(std::string_view file, std::string_view function, int line,
              std::string_view expression)
{
    PrintErrorHeaderMessage(Severity_Error, file, function, line, expression);
}

} // namespace eng::err::intl
