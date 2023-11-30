#pragma once
#include <thread>
#include <memory>
#include "readlog_config.h"

namespace tapdata
{
    class LogSubprocess
    {
    public:
        LogSubprocess(std::shared_ptr<tapdata::readlog_config> config);
        ~LogSubprocess();

        bool start() noexcept;
        void stop() noexcept;


        std::shared_ptr<tapdata::readlog_config> readlog_config() const
        {
            return readlog_config_;
        }

    private:
        std::shared_ptr<tapdata::readlog_config> readlog_config_;
        int status_{};
        int wait_return_{};
        int pid_{};
        std::thread wait_thread_;
    };
}