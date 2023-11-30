#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sstream>
#include <chrono>
#include "LogSubprocess.h"
#include "tool.h"
#include "tapdata_base64.h"
#include "log_imp.h"
#include "DB2PlugInDataApp.h"
#include "DB2CataLog.h"

namespace tapdata
{
	using namespace std;
	LogSubprocess::LogSubprocess(std::shared_ptr<tapdata::readlog_config> config) :readlog_config_{ config }
	{
	}

	LogSubprocess::~LogSubprocess()
	{
		stop();
		if (wait_thread_.joinable())
			wait_thread_.join();
	}

	bool LogSubprocess::start() noexcept
	{

		using namespace google::protobuf::util;
		string ser_str;
		if (!readlog_config_->readLogRequest_.SerializeToString(&ser_str))
		{
			LOG_ERROR("SerializeToString fail!");
			return false;
		}
		const string args_str = tool::base64_encode(ser_str.data(), ser_str.size());

		const auto node_alias = DB2CataLog::create_connect(readlog_config_->readLogRequest_.source().databasehostname(),
			readlog_config_->readLogRequest_.source().databaseservicename(), readlog_config_->readLogRequest_.source().databasename());
		if (node_alias.first.empty() || node_alias.second.empty())
			return false;

		const auto path = tool::get_app_path();
		const auto pname = tool::get_process_name();
		const auto server_address = get_app<DB2PlugInDataApp>()->get_local_config()->get_server_config().server_address_;
		pid_ = fork();
		if (pid_ < 0)
		{
			readlog_config_->log_state_ = decltype(readlog_config_->log_state_)::stop_by_error;
			return false;
		}
		else if (pid_ > 0)
		{
			wait_thread_ = thread([this, node_alias]()
				{
					int kill_try_times{};
					int kill_result{};
					while (true)
					{
						if (decltype(readlog_config_->log_state_)::running != readlog_config_->log_state_)
						{
							if (pid_ > 0)
							{
								kill_result = kill(pid_, kill_try_times++ > 9 ? SIGKILL : SIGTERM);
								if (kill_result != 0)
								{
									LOG_WARN("kill return:{}", kill_result);
								}
								this_thread::sleep_for(1s);
							}
						}
						wait_return_ = waitpid(pid_, &status_, WNOHANG);
						if (wait_return_ == 0)
							this_thread::sleep_for(1s);
						else if (wait_return_ == pid_)
						{
							pid_ = -1;
							if (readlog_config_->log_state_ == decltype(readlog_config_->log_state_)::running)
								readlog_config_->log_state_ = decltype(readlog_config_->log_state_)::stop_by_error;
							break;
						}
						else
						{
							LOG_ERROR("waitpid fail! result:", wait_return_);
							break;
						}
					}

					DB2CataLog::delete_connect(node_alias);
				});
			return true;
		}
		else
		{
			int re = execl(path.c_str(), pname.c_str(), "C", args_str.c_str(), node_alias.first.c_str(), node_alias.second.c_str(),
				server_address.c_str(), NULL);
			if (re == -1)
				exit(re);
			return true;
		}

		return false;
	}

	void LogSubprocess::stop() noexcept
	{
		if (readlog_config_->log_state_ == decltype(readlog_config_->log_state_)::running)
			readlog_config_->log_state_ = decltype(readlog_config_->log_state_)::shutdown;
	}
}
