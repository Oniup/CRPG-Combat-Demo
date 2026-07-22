#pragma once

#include "engine/utilities/TypeInfo.h"

#define ENG_RESOURCE(_resource_class)                                                              \
public:                                                                                            \
    unsigned GetInstanceTypeHash() const override                                                  \
    {                                                                                              \
        return eng::TypeInfo<_resource_class>::GetHash();                                          \
    }                                                                                              \
    std::string_view GetInstanceTypeName() const override                                          \
    {                                                                                              \
        return eng::TypeInfo<_resource_class>::GetName();                                          \
    }                                                                                              \
                                                                                                   \
private:

namespace eng {

class Resource
{
public:
    virtual ~Resource() {}

    virtual unsigned GetInstanceTypeHash() const         = 0;
    virtual std::string_view GetInstanceTypeName() const = 0;
};

} // namespace eng
