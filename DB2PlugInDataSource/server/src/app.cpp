#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include "app.h"
#include "log_imp.h"
#include "multiprocess_mutex.h"

namespace tapdata
{
    using namespace std;
    void sig_term_proc(int sig_no);
    void sig_crash_proc(int sig_no);
    void sig_dump_proc(int sig_no);

    void add_signal_proc()
    {
        // 增加信号处理
        signal(SIGINT, sig_term_proc);
        signal(SIGPIPE, sig_term_proc);
        //signal(SIGSEGV, sig_crash_proc);
        signal(SIGTERM, sig_term_proc);
        signal(SIGABRT, sig_term_proc);
        signal(SIGUSR1, sig_dump_proc);
    }

    struct message_loop
    {
        app* lapp_{};
        int quit_reason_{};
        bool keep_running_{ true };

        mutex mutex_;
        condition_variable condition_variable_;

        int exec(bool is_s = false)
        {
	    init_multi_process_mutex(is_s);
            unique_lock<decltype(mutex_)> lock{ mutex_ };
            if (keep_running_)
            {
                lapp_->run();
                condition_variable_.wait(lock);
            }
            LOG_INFO("app quit!, reason:{}", quit_reason_);
            if (is_s) {
                destroy_multi_process_mutex();
            }
            return quit_reason_;
        }

        void quit(int resion)
        {
            quit_reason_ = resion;
            keep_running_ = false;
            // 这里通知条件变量后，进程就退出了，会导致事务数据丢失。
            // condition_variable_.notify_one();
        }

        void notify_one()
        {
            condition_variable_.notify_one();
        }

        void sig_term_proc(int sig_no)
        {
            lapp_->sig_term_proc(sig_no);
        }

        void sig_crash_proc(int sig_no)
        {
            lapp_->sig_crash_proc(sig_no);
        }

        void sig_dump_proc(int sig_no)
        {
            lapp_->sig_dump_proc(sig_no);
        }
    };

    message_loop mess_loop;

    app* get_app()
    {
        return mess_loop.lapp_;
    }

    app::app() noexcept
    {
        mess_loop.lapp_ = this;
        add_signal_proc();
    }

    app::~app() noexcept
    {
        mess_loop.lapp_ = nullptr;
    }

    void app::quit(int reason) noexcept
    {
        mess_loop.quit(reason);
    }

    void app::notify_one() noexcept
    {
        mess_loop.notify_one();
    }

    int app::exec(bool is_s) noexcept
    {
        return mess_loop.exec(is_s);
    }

    bool app::keep_run() noexcept
    {
        return mess_loop.keep_running_;
    }

    void sig_term_proc(int sig_no)
    {
        LOG_INFO("app recv signal {}, pid:{}", sig_no, getpid());
        return mess_loop.sig_term_proc(sig_no);
    }

    void sig_crash_proc(int sig_no)
    {
        LOG_INFO("app recv crash {}", sig_no);
        return mess_loop.sig_crash_proc(sig_no);
    }

    void sig_dump_proc(int sig_no)
    {
        LOG_INFO("app recv dump {}", sig_no);
        return mess_loop.sig_dump_proc(sig_no);
    }
}