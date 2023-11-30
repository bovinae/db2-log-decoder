#pragma once
#include <mutex>
#include <memory>
#include "readlog_config.h"
#include "ErrorCode.h"
#include "ReadLogManager.h"
#include "LogSubprocess.h"

namespace tapdata
{
    class readlog_config_manager
    {
    public:
        readlog_config_manager();
        ~readlog_config_manager();

        //OK ALREADY_CREATE INIT_ERR
        ErrorCode start(std::shared_ptr<readlog_config> config) noexcept;

        //OK NOT_EXIST
        ErrorCode stop(const std::string& task_id) noexcept;

        //OK NOT_EXIST PAUSED
        ErrorCode pause(const std::string& task_id) noexcept;

        //OK NOT_EXIST RUNNING
        ErrorCode resume(const std::string& task_id) noexcept;

        //OK NOT_EXIST
        ErrorCode pullReadLog(int32_t protocolversion, const std::string& task_id, std::unique_ptr<ReadLogResponse>& resp) noexcept;

        //may empty
        std::vector<std::shared_ptr<readlog_config>> list_all() noexcept;

        //may return nullptr
        std::shared_ptr<readlog_config> get(const std::string& task_id) noexcept;

        void pushReadLog(PushReadLogRequest&& req, std::unique_ptr<PushReadLogResponse>& resp) noexcept;

    private:
        mutable std::mutex mutex_;
        std::vector<std::shared_ptr<LogSubprocess>> sproces_;
    };
}