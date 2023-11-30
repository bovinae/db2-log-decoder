#include <thread>
#include <string>
#include <sstream>
#include <cstring>
#include "log_imp.h"
#include "DB2PlugInDataApp.h"
#include "DB2ReadLogApp.h"
#include "tool.h"
#include "tool_extern.h"
#include "tapdata_base64.h"
#include "version.h"

using namespace std;

int main(int argc, char* argv[])
{
	// 192.168.1.132 root/Gotapd8!!
	// db2inst1/Gotapd8!
	// db2 catalog tcpip4 node NODEDDL remote 192.168.1.132 server 50000
	// db2 catalog database TESTDB as TDDL at node NODEDDL
	// db2 connect to TDDL user db2inst1 using \"Gotapd8\!\"
	// eg: db210_src_srv C CgIIARoPMTM5ODQ5OTE5ODA5NjY0MgIIBDoxCAISBlRFU1REQhoIZGIyaW5zdDEiBWFkbWluKg0xOTIuMTY4LjEuMTMyMgU1MDAwMA== ETKLCQNF NHQNMZMJ 0.0.0.0:50051
	if (argc == 6)
	{
		stringstream st;
		st << this_thread::get_id();
		LOG_INIT("DB2ReadLogC" + st.str(), tool::get_process_name() + "_clogs");
		LOG_SET_LEVEL_AND_OUTPUT((spdlog::level::level_enum)1, false);
		if (memcmp(argv[1], "C", sizeof("C")) != 0)
		{
			LOG_ERROR("argv[1] error!");
			return -1;
		}
		else
		{
			LOG_INFO("DB2ReadLogApp running");
		}

		using namespace google::protobuf::util;
		tapdata::ReadLogRequest readLogRequest;

		if (!argv[2] || !argv[3] || !argv[4] || !argv[5])
		{
			LOG_ERROR("check argv, param error");
			return -1;
		}
		const auto vec = tool::base64_decode(argv[2], strlen(argv[2]));
		if (!readLogRequest.ParseFromString(std::string(vec.data(), vec.size())))
		{
			LOG_ERROR("check read log request, param error");
			return -1;
		}
		
		//this_thread::sleep_for(20s);
		try
		{
			LOG_INFO("init remote big endian:{}", readLogRequest.bigendian() ? "true" : "false");
			tool::init_remote_big_endian(readLogRequest.bigendian());
			tapdata::DB2ReadLogApp app{ move(readLogRequest), argv[3], argv[4], argv[5] };
			return app.exec();
		}
		catch (const exception& ex)
		{
			LOG_ERROR("OraclePlugInDataApp err:{}", ex.what());
			return -1;
		}
		return 0;
	}
	else if (argc == 1)
	{
		LOG_INIT("DB2ReadLogServer", tool::get_process_name() + "_slogs");
		LOG_SET_LEVEL_AND_OUTPUT((spdlog::level::level_enum)1, false);
		try
		{
			tapdata::DB2PlugInDataApp app;
			LOG_INFO("DB2PlugInDataApp running");
			LOG_INFO("version:{}", tapdata::LocalServerInfo);
			return app.exec();
		}
		catch (const exception& ex)
		{
			LOG_ERROR("OraclePlugInDataApp err:{}", ex.what());
			return -1;
		}

		return 0;
	}
	else
	{
		LOG_INIT("DB2ReadLogServer", tool::get_process_name() + "_elogs");
		LOG_SET_LEVEL_AND_OUTPUT((spdlog::level::level_enum)1, false);

		LOG_ERROR("argv error! check argv, argc:{}", argc);
		return -1;
	}
}