#pragma once
#define TAPDATA_EXTERN_JSON_STR(X) extern const char* json_##X

namespace tapdata
{
    TAPDATA_EXTERN_JSON_STR(version);

    TAPDATA_EXTERN_JSON_STR(app_log_config);
    TAPDATA_EXTERN_JSON_STR(console);
    TAPDATA_EXTERN_JSON_STR(level);
    TAPDATA_EXTERN_JSON_STR(jump_size);

    TAPDATA_EXTERN_JSON_STR(server_config);
    TAPDATA_EXTERN_JSON_STR(loop_wait_ms);
    TAPDATA_EXTERN_JSON_STR(accept_threads);
    TAPDATA_EXTERN_JSON_STR(qps);
    TAPDATA_EXTERN_JSON_STR(server_address);

}
#undef TAPDATA_EXTERN_JSON_STR
