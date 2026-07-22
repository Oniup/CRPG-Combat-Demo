#pragma once

#include "fmt/base.h"
#include "fmt/format.h"

namespace fmt {

template <>
struct formatter<Vector3>
{
    constexpr auto parse(fmt::format_parse_context& context) { return context.begin(); }
    template <typename FormatContext>
    auto format(const Vector3& vec, FormatContext& context) const
    {
        return fmt::format_to(context.out(), "[{}, {}, {}]", vec.x, vec.y, vec.z);
    }
};

} // namespace fmt
