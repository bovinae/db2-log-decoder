#pragma once
#include <thread>
#include <memory>
#include "local_app.h"
#include "DB2PlugInDataSource.pb.h"

namespace tapdata
{
    class DB2ReadLogApp final : public local_app
    {
    public:
        DB2ReadLogApp(ReadLogRequest&&, const char* node_name, const char* db_alias, const char* server_address);
        ~DB2ReadLogApp();

    protected:
        void run() noexcept override;
        void sig_term_proc(int sig_no) override;
        void sig_crash_proc(int sig_no) override;
        void sig_dump_proc(int sig_no) override;

    private:
        void thread_func() noexcept;

    private:
        ReadLogRequest readLogRequest_;
        std::string node_name_;
        std::string db_alias_;
        std::string server_address_;
        std::thread thread_;
    };
}