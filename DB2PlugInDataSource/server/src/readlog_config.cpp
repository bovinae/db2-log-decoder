#include <thread>
#include "readlog_config.h"
#include "log_imp.h"

namespace tapdata
{
	using namespace std;
	int32_t readlog_config::pushReadLogResponse(ReadLogResponse&& resp)
	{
		if (log_state_ == log_state::running)
		{
			if (readlog_time_out_s_ < chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now().time_since_epoch()).count())
				return -1;
			const auto size = resps_.size();
			if (size < 0x400)
			{
				resps_.put(move(resp));
				return 0;
			}
			else
			{
				return size << 2;
			}
		}
		return -1;
	}

	readlog_config::pop_log_state readlog_config::try_wait_and_pop(ReadLogResponse& x, uint32_t _milli)
	{
		pop_log_state st{ pop_log_state::ok };
		readlog_time_out_s_ = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now().time_since_epoch()).count()+ 86400;
		if (pause_ || !resps_.try_wait_and_pop(x, _milli))//如果暂停了或者没取到数据
		{
			if (log_state_ == decltype(log_state_)::running)
				st = decltype(st)::empty;
			else if (log_state_ == decltype(log_state_)::shutdown)
				st = decltype(st)::shutdown;
			else if (log_state_ == decltype(log_state_)::stop_by_error)
				st = decltype(st)::stop_by_error;
			else
			{
				LOG_ERROR("logic error , check");
				assert(false);
			}
		}
		return st;
	}

}
