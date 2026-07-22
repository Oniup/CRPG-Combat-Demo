#pragma once

#include "raylib.h"
#include <string_view>

namespace eng {

Color ConvertHexToColor(const std::string_view& rgb_hex);

} // namespace eng
