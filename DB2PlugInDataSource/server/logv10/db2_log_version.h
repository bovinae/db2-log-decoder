#pragma once
#include <cstdint>
namespace tapdata
{
    enum class db2_log_version :int32_t
    {
        AUTO = 0, v9 = 1, v10 = 2
    };

    extern const db2_log_version LocalDB2LogVersion;

    enum class db2_log_payload_version :int32_t
    {
        V1
    };

}