#include "local_app.h"
#include "log_imp.h"
#include "tool.h"

namespace tapdata
{
    using namespace std;

    local_app::local_app() noexcept : app()
    {
        do
        {
            auto config_folder = tool::get_exe_path() + tool::get_process_name() + "_config/";
            if (!tool::create_directories(config_folder.c_str()))
            {
                LOG_ERROR("make folder error");
                break;
            }
            local_config_ = new(nothrow) local_config(config_folder + "config.json");
            if (!local_config_)
            {
                LOG_ERROR("config error in path:{}", config_folder);
                break;
            }
            LOG_SET_LEVEL_AND_OUTPUT((spdlog::level::level_enum)local_config_->get_app_log_config().level_, local_config_->get_app_log_config().console_);

            return;
        } while (false);
        quit(-1);
    }

    local_app::~local_app() noexcept
    {
        delete local_config_;
    }

    const local_config* local_app::get_local_config() noexcept
    {
        return local_config_;
    }

    void local_app::sig_term_proc(int sig_no)
    {
        LOG_INFO("sig_term_proc, sig_no:{}", sig_no);
        quit(sig_no);
    }

    void local_app::sig_crash_proc(int sig_no)
    {
        quit(sig_no);
    }

    void local_app::sig_dump_proc(int sig_no)
    {
        quit(sig_no);
    }
}

