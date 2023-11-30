#pragma once
#include <string>
#include "app.h"
#include "local_config.h"

namespace tapdata
{
    class local_app : public app
    {
    public:
        local_app() noexcept;
        ~local_app() noexcept;

    public:
        const local_config* get_local_config() noexcept;

    protected:
        virtual void run() noexcept = 0;
        virtual void sig_term_proc(int sig_no) = 0;
        virtual void sig_crash_proc(int sig_no) = 0;
        virtual void sig_dump_proc(int sig_no) = 0;

    protected:
        local_config* local_config_{};
    };
}