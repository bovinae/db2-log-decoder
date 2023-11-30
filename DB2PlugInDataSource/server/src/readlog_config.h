#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <chrono>
#include <atomic>
#include "blocking_queue.h"
#include "DB2PlugInDataSource.pb.h"

namespace tapdata
{
    struct readlog_config
    {
        ReadLogRequest readLogRequest_;

        enum class log_state { running, shutdown, stop_by_error };
        bool pause_{ false };
        log_state log_state_{ log_state::running };
        std::string err_msg_;

        //<0 需要停止，
        //==0 OK
        //>0 整数代表 需要重发
        int32_t pushReadLogResponse(ReadLogResponse&& resp);

        enum class pop_log_state { ok, empty, shutdown, stop_by_error };
        pop_log_state try_wait_and_pop(ReadLogResponse& x, uint32_t _milli);

    private:
        blocking_queue<ReadLogResponse> resps_;
        std::atomic<int64_t> readlog_time_out_s_{ std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count() + 86400 };
    };
}