#pragma once
#include <cstdint>
namespace tapdata
{
    enum class LocalProtocolVersion :int32_t
    {
        v0, v1
    };

    enum class LocalServerVersion :int32_t
    {
        v0, v1
    };

    extern const char* const LocalServerInfo;

}