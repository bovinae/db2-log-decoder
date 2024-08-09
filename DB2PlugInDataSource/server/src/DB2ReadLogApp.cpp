#include <thread>
#include <algorithm>
#include <grpcpp/create_channel.h>

#include "DB2ReadLogApp.h"
#include "version.h"
#include "tool.h"
#include "log_imp.h"
#include "dblogconn.h"
#include "PushReadLogClient.h"

namespace tapdata
{
	using namespace std;
	DB2ReadLogApp::DB2ReadLogApp(ReadLogRequest&& req, const char* node_name, const char* db_alias, const char* server_address) :
		local_app(), readLogRequest_(move(req)), node_name_(node_name), db_alias_(db_alias),
		server_address_(server_address)
	{
	}

	DB2ReadLogApp::~DB2ReadLogApp()
	{
		if (thread_.joinable())
			thread_.join();
	}

	void DB2ReadLogApp::run() noexcept
	{
		thread_ = thread{ &DB2ReadLogApp::thread_func, this };
	}

	void DB2ReadLogApp::sig_term_proc(int sig_no)
	{
		local_app::sig_term_proc(sig_no);
	}

	void DB2ReadLogApp::sig_crash_proc(int sig_no)
	{
		local_app::sig_crash_proc(sig_no);
	}

	void DB2ReadLogApp::sig_dump_proc(int sig_no)
	{
		local_app::sig_dump_proc(sig_no);
	}

	void DB2ReadLogApp::thread_func() noexcept
	{
		grpc::ChannelArguments channel_args;
		channel_args.SetInt(GRPC_ARG_MAX_SEND_MESSAGE_LENGTH, 100 * 1024 * 1024);//100M
		auto channel = grpc::CreateCustomChannel(
			server_address_.c_str(), grpc::InsecureChannelCredentials(), channel_args);

		ConnectDbSet connectDbSet{};

		strncpy(connectDbSet.alias, db_alias_.data(), sizeof(connectDbSet.alias) - 1);
		strncpy(connectDbSet.nodeName, node_name_.data(), sizeof(connectDbSet.nodeName) - 1);
		strncpy(connectDbSet.user, readLogRequest_.source().databaseusername().data(), sizeof(connectDbSet.user) - 1);
		strncpy(connectDbSet.pswd, readLogRequest_.source().databasepassword().data(), sizeof(connectDbSet.pswd) - 1);


		ReadLogWrap readLogWrap(readLogRequest_);

		{
			PushReadLogClient client(readLogRequest_, channel);
			readLogWrap.set_message_callback_funcs(
				{
					std::bind(&PushReadLogClient::push_dml, &client, placeholders::_1, placeholders::_2),
					std::bind(&PushReadLogClient::push_ddl, &client, placeholders::_1, placeholders::_2),

					std::bind(&PushReadLogClient::push_undo_dml, &client, placeholders::_1),
					std::bind(&PushReadLogClient::push_undo_ddl, &client, placeholders::_1),

					std::bind(&PushReadLogClient::push_reorg_table, &client, placeholders::_1),

					std::bind(&PushReadLogClient::push_commit, &client, placeholders::_1),
					std::bind(&PushReadLogClient::push_heartbeat, &client, placeholders::_1),
					std::bind(&PushReadLogClient::push_abort, &client, placeholders::_1),

					std::bind(&PushReadLogClient::flash_push, &client, placeholders::_1)
				}
			);
			auto result = readLogWrap.db2_read_log(connectDbSet, readLogRequest_.scn(), 1000);
			LOG_INFO("db2 log back result:{}", result);
		} // WrapPushReadLogRequest里面的析构函数会等待异步任务退出
		// quit(result);
		notify_one(); // 唤醒父线程中的条件等待
		// 当前线程（子线程）退出
	}
}

