#include <algorithm>
#include "readlog_config_manager.h"
#include "log_imp.h"
#include "DB2PlugInDataApp.h"

namespace tapdata
{
    using namespace std;

    readlog_config_manager::readlog_config_manager()
    {
    }

    readlog_config_manager::~readlog_config_manager()
    {
        lock_guard<decltype(mutex_)> lck{ mutex_ };
        for (auto g : sproces_)
            g->stop();
    }

    ErrorCode readlog_config_manager::start(std::shared_ptr<readlog_config> config) noexcept
    {
        ErrorCode back{ ErrorCode::ALREADY_CREATE };
        {
            lock_guard<decltype(mutex_)> lck{ mutex_ };
            if (!any_of(sproces_.begin(), sproces_.end(), [config](std::shared_ptr<LogSubprocess> g)
                {
                    return g->readlog_config()->readLogRequest_.id() == config->readLogRequest_.id();
                }))
            {
                std::shared_ptr<LogSubprocess> l = make_shared<LogSubprocess>(config);
                sproces_.push_back(l);
                back = l->start() ? decltype(back)::OK : decltype(back)::INIT_ERR;
            }
        }
        return back;
    }

    ErrorCode readlog_config_manager::stop(const std::string& task_id) noexcept
    {
        ErrorCode back{ ErrorCode::NOT_EXIST };
        {
            lock_guard<decltype(mutex_)> lck{ mutex_ };
            if (auto it = find_if(sproces_.begin(), sproces_.end(), [&task_id](std::shared_ptr<LogSubprocess> g)
                {
                    return g->readlog_config()->readLogRequest_.id() == task_id;
                }); it != sproces_.end())
            {
                it[0]->stop();
                std::swap(*it, sproces_.back());
                sproces_.pop_back();
                back = decltype(back)::OK;
            }
        }
        return back;
    }

    ErrorCode readlog_config_manager::pause(const std::string& task_id) noexcept
    {
        ErrorCode back{ ErrorCode::NOT_EXIST };
        {
            lock_guard<decltype(mutex_)> lck{ mutex_ };
            if (auto it = find_if(sproces_.begin(), sproces_.end(), [&task_id](std::shared_ptr<LogSubprocess> g)
                {
                    return g->readlog_config()->readLogRequest_.id() == task_id;
                }); it != sproces_.end())
            {
                if (it[0]->readlog_config()->pause_)
                    back = decltype(back)::PAUSED;
                else
                {
                    it[0]->readlog_config()->pause_ = true;
                    back = decltype(back)::OK;
                }
            }
        }
        return back;
    }

    ErrorCode readlog_config_manager::resume(const std::string& task_id) noexcept
    {
        ErrorCode back{ ErrorCode::NOT_EXIST };
        {
            lock_guard<decltype(mutex_)> lck{ mutex_ };
            if (auto it = find_if(sproces_.begin(), sproces_.end(), [&task_id](std::shared_ptr<LogSubprocess> g)
                {
                    return g->readlog_config()->readLogRequest_.id() == task_id;
                }); it != sproces_.end())
            {
                if (it[0]->readlog_config()->pause_)
                    back = decltype(back)::RUNNING;
                else
                {
                    it[0]->readlog_config()->pause_ = false;
                    back = decltype(back)::OK;
                }
            }
        }
        return back;
    }

    ErrorCode readlog_config_manager::pullReadLog(int32_t protocolversion, const std::string& task_id, std::unique_ptr<ReadLogResponse>& resp) noexcept
    {
        ErrorCode back{ ErrorCode::NOT_EXIST };
        decltype(sproces_.front()->readlog_config()) config;
        {
            lock_guard<decltype(mutex_)> lck{ mutex_ };
            if (auto it = find_if(sproces_.begin(), sproces_.end(), [&task_id](std::shared_ptr<LogSubprocess> g)
                {
                    return g->readlog_config()->readLogRequest_.id() == task_id;
                }); it != sproces_.end())
            {
                config = it[0]->readlog_config();
            }
        }

        if (!config)
            return back;

        back = decltype(back)::OK;

        decltype(config->try_wait_and_pop(*resp, 200)) re;
        while (true)
        {
            re = config->try_wait_and_pop(*resp, 200);
            if (re == decltype(re)::ok)
            {
                resp->set_code(ResponseCode::OK);
                break;
            }
            else if (re == decltype(re)::stop_by_error)
            {
                resp->set_code(ResponseCode::STOP_BY_EXCEPTION);
                break;
            }
            else if (re == decltype(re)::shutdown)
            {
                resp->set_code(ResponseCode::PASSIVE_STOP);
                break;
            }
            else if (re == decltype(re)::empty)
            {
                if (get_app<DB2PlugInDataApp>()->keep_run())
                {
                    this_thread::sleep_for(200ms);
                    continue;
                }
                else
                {
                    resp->set_code(ResponseCode::SHUTTING_DOWN);
                    break;
                }
            }
            else
            {
                LOG_ERROR("logic error, check");
                assert(false);
            }
        }
        resp->mutable_header()->set_protocolversion(protocolversion);

        return back;
    }

    std::vector<std::shared_ptr<readlog_config>> readlog_config_manager::list_all() noexcept
    {
        vector<shared_ptr<readlog_config>> back;
        {
            lock_guard<decltype(mutex_)> lck{ mutex_ };
            for (const auto& it : sproces_)
                back.emplace_back(it->readlog_config());
        }
        return back;
    }

    std::shared_ptr<readlog_config> readlog_config_manager::get(const std::string& task_id) noexcept
    {
        shared_ptr<readlog_config> back;
        {
            lock_guard<decltype(mutex_)> lck{ mutex_ };
            if (auto it = find_if(sproces_.begin(), sproces_.end(), [&task_id](std::shared_ptr<LogSubprocess> g)
                {
                    return g->readlog_config()->readLogRequest_.id() == task_id;
                }); it != sproces_.end())
            {
                back = it[0]->readlog_config();
            }
        }
        return back;
    }

    void readlog_config_manager::pushReadLog(PushReadLogRequest&& req, std::unique_ptr<PushReadLogResponse>& resp) noexcept
    {
        decltype(sproces_.front()->readlog_config()) config;
        {
            lock_guard<decltype(mutex_)> lck{ mutex_ };
            if (auto it = find_if(sproces_.begin(), sproces_.end(), [&req](std::shared_ptr<LogSubprocess> g)
                {
                    return g->readlog_config()->readLogRequest_.id() == req.id();
                }); it != sproces_.end())
            {
                config = it[0]->readlog_config();
            }
        }

        if (!config)
        {
            resp->set_code(PushResponseCode::PUSH_STOP);
            return;
        }

        auto re = config->pushReadLogResponse(move(*req.mutable_logresponse()));
        if (re < 0)
        {
            resp->set_code(PushResponseCode::PUSH_STOP);
        }
        else if (re == 0)
        {
            resp->set_code(PushResponseCode::PUSH_OK);
        }
        else
        {
            resp->set_code(PushResponseCode::PUSH_PAUSED);
            resp->set_waittimems(re);
        }
    }
}

