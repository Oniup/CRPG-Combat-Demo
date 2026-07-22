#include "engine/utilities/Color.h"

#include "engine/core/Error.h"
#include <cstdint>

namespace eng::intl {

uint8_t ConvertHexNibbleToValue(char val)
{
    if (val >= '0' && val <= '9')
        return val - '0';
    if (val >= 'A' && val <= 'Z')
        return val - 'A' + 10;
    if (val >= 'a' && val <= 'z')
        return val - 'a' + 10;
    return 255; // error value
}

uint8_t ConvertHexDigitToByte(const std::string_view& hex)
{
    ASSERT(
        hex.length() == 2, "hexadecimal digit length cannot be larger or smaller than 2 '{}'", hex);

    uint8_t upperNibble = ConvertHexNibbleToValue(hex[0]);
    uint8_t lowerNibble = ConvertHexNibbleToValue(hex[1]);
    if (upperNibble == 255 || lowerNibble == 255)
        FATAL("Invalid hex digit format: {}", hex);

    return upperNibble * 16 + lowerNibble;
}

} // namespace eng::intl

namespace eng {

Color ConvertHexToColor(const std::string_view& hexColor)
{
    size_t offset       = hexColor.starts_with('#') ? 1 : 0;
    size_t sourceLength = hexColor.length() - offset;

    ASSERT(!hexColor.empty() && sourceLength >= 6,
           "Invalid color format {}, must be in format '#ffffff'",
           hexColor);

    uint8_t r, g, b, a = 255;
    r = intl::ConvertHexDigitToByte(hexColor.substr(offset + 0, 2));
    g = intl::ConvertHexDigitToByte(hexColor.substr(offset + 2, 2));
    b = intl::ConvertHexDigitToByte(hexColor.substr(offset + 4, 2));
    if (sourceLength == 8)
        a = intl::ConvertHexDigitToByte(hexColor.substr(offset + 6, 2));

    return Color(r, g, b, a);
}

} // namespace eng
