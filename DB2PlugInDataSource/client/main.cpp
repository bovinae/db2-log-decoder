#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include <future>
#include <signal.h>
#include <sstream>

#include <grpcpp/grpcpp.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <google/protobuf/text_format.h>


#include "DB2PlugInDataSource.grpc.pb.h"
#include "finally_imp.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

using namespace tapdata;
const char map16[17] = "0123456789abcdef";
class DemoClient {
public:
	DemoClient(std::shared_ptr<Channel> channel)
		: stub_(DB2ReadLogServer::NewStub(channel)) {}

	void Ping() {
		PingResponse resp;

		// Context for the client. It could be used to convey extra information to
		// the server and/or tweak certain RPC behaviors.
		ClientContext context;

		// The actual RPC.
		PingRequest req;
		req.mutable_header()->set_protocolversion(1);

		Status status = stub_->Ping(&context, req, &resp);

		// Act upon its status.
		if (status.ok()) {
			std::cout << "Ping recv, ProtocolVersion:" << resp.header().protocolversion() << ", status:" << resp.status() << ", msg:" << resp.msg() << std::endl;
		}
		else {
			std::cout << status.error_code() << ": " << status.error_message()
				<< std::endl;
		}
	}

	void ServerInfo() {
		ServerInfoResponse resp;

		// Context for the client. It could be used to convey extra information to
		// the server and/or tweak certain RPC behaviors.
		ClientContext context;

		// The actual RPC.
		google::protobuf::Empty req;


		Status status = stub_->ServerInfo(&context, req, &resp);

		// Act upon its status.
		if (status.ok()) {
			std::cout << "ServerInfo recv, ProtocolVersion:" << resp.header().protocolversion() <<
				", serverVersion:" << resp.serverversion() << ", info:" << resp.info() << ", supportProtocolVersion:";
			for (const auto i : resp.supportprotocolversion())
				std::cout << i << " ";
			std::cout << ", supportWriterType:";

			for (const auto i : resp.supportwritertype())
				std::cout << tapdata::WriterType_Name(i) << " ";
			std::cout << std::endl;
		}
		else {
			std::cout << status.error_code() << ": " << status.error_message()
				<< std::endl;
		}
	}

	void CreateReadlogTaskV10(const std::string& id, std::vector<int>&& table_ids, int64_t start_time, const std::string& scn, const std::string& host, const std::string& port, const std::string& db, const std::string& passwd, bool bigendian, bool cache_lri) {
		ControlResponse resp;

		// Context for the client. It could be used to convey extra information to
		// the server and/or tweak certain RPC behaviors.
		ClientContext context;


		// The actual RPC.
		ReadLogRequest req;
		req.mutable_header()->set_protocolversion(1);
		req.set_id(id);

#if 0
		req.mutable_source()->set_databasename("TDDEV");
		req.mutable_source()->set_databaseusername("db2inst1");
		req.mutable_source()->set_databasepassword("Ikasinfo123#");
		req.mutable_source()->set_databaseversion(DB2Veresion::V10);
		req.mutable_source()->set_databasehostname("192.168.11.136");
		req.mutable_source()->set_databaseservicename("50000");
		req.mutable_tables()->Add(std::move(sourceTable));
		req.set_bigendian(true);
#else

		req.mutable_source()->set_databasename(db); // easton TESTDB
		req.mutable_source()->set_databaseusername("db2inst1");
		req.mutable_source()->set_databasepassword(passwd);
		req.mutable_source()->set_databaseversion(DB2Veresion::V10);
		req.mutable_source()->set_databasehostname(host); // 192.168.1.132 172.17.0.3
		req.mutable_source()->set_databaseservicename(port); // "50008"

		SourceTable table;
		for (auto& id : table_ids)
		{
			table.set_tableid(id);
			req.mutable_tables()->Add(std::move(table));
		}


		req.set_bigendian(bigendian);
		req.set_cachelri(cache_lri);

		req.set_stime(start_time);
#endif
		if (!scn.empty())
			req.set_scn(scn);

		Status status = stub_->CreateReadLogTask(&context, req, &resp);

		// Act upon its status.
		if (status.ok()) {
			std::cout << "CreateReadLogTask recv, ProtocolVersion:" << resp.header().protocolversion() << ", code:" << tapdata::ResponseCode_Name(resp.code()) <<
				", msg:" << resp.msg() << std::endl;
		}
		else {
			std::cout << status.error_code() << ": " << status.error_message()
				<< std::endl;
		}
	}

	void CreateReadlogTaskV09(const std::string& id, std::vector<int>&& table_ids, int64_t start_time, const std::string& scn) {
		ControlResponse resp;

		// Context for the client. It could be used to convey extra information to
		// the server and/or tweak certain RPC behaviors.
		ClientContext context;


		// The actual RPC.
		ReadLogRequest req;
		req.mutable_header()->set_protocolversion(1);
		req.set_id(id);
		req.mutable_source()->set_databasename("TESTLOG");
		req.mutable_source()->set_databaseusername("db2inst1");
		req.mutable_source()->set_databasepassword("admin");
		req.mutable_source()->set_databaseversion(DB2Veresion::V9);
		req.mutable_source()->set_databasehostname("192.168.1.132");
		req.mutable_source()->set_databaseservicename("40000");

		SourceTable table;
		for (auto& id : table_ids)
		{
			table.set_tableid(id);
			req.mutable_tables()->Add(std::move(table));
		}

		req.set_stime(start_time);

		if (!scn.empty())
			req.set_scn(scn);

		Status status = stub_->CreateReadLogTask(&context, req, &resp);

		// Act upon its status.
		if (status.ok()) {
			std::cout << "CreateReadLogTask recv, ProtocolVersion:" << resp.header().protocolversion() << ", code:" << tapdata::ResponseCode_Name(resp.code()) <<
				", msg:" << resp.msg() << std::endl;
		}
		else {
			std::cout << status.error_code() << ": " << status.error_message()
				<< std::endl;
		}
	}

	void DeleteReadLogTask(const std::string& id) {
		ControlResponse resp;

		// Context for the client. It could be used to convey extra information to
		// the server and/or tweak certain RPC behaviors.
		ClientContext context;

		// The actual RPC.
		TaskHandleRequest req;
		req.mutable_header()->set_protocolversion(1);
		req.set_id(id);

		Status status = stub_->DeleteReadLogTask(&context, req, &resp);

		// Act upon its status.
		if (status.ok()) {
			std::cout << "DeleteReadLogTask recv, ProtocolVersion:" << resp.header().protocolversion() << ", code:" << tapdata::ResponseCode_Name(resp.code()) <<
				", msg:" << resp.msg() << std::endl;
		}
		else {
			std::cout << status.error_code() << ": " << status.error_message()
				<< std::endl;
		}
	}

	void PauseReadLogTask(const std::string& id) {
		ControlResponse resp;

		// Context for the client. It could be used to convey extra information to
		// the server and/or tweak certain RPC behaviors.
		ClientContext context;

		// The actual RPC.
		TaskHandleRequest req;
		req.mutable_header()->set_protocolversion(1);
		req.set_id(id);

		Status status = stub_->PauseReadLogTask(&context, req, &resp);

		// Act upon its status.
		if (status.ok()) {
			std::cout << "PauseReadLogTask recv, ProtocolVersion:" << resp.header().protocolversion() << ", code:" << tapdata::ResponseCode_Name(resp.code()) <<
				", msg:" << resp.msg() << std::endl;
		}
		else {
			std::cout << status.error_code() << ": " << status.error_message()
				<< std::endl;
		}
	}

	void ResumeReadLogTask(const std::string& id) {
		ControlResponse resp;

		// Context for the client. It could be used to convey extra information to
		// the server and/or tweak certain RPC behaviors.
		ClientContext context;

		// The actual RPC.
		TaskHandleRequest req;
		req.mutable_header()->set_protocolversion(1);
		req.set_id(id);

		Status status = stub_->ResumeReadLogTask(&context, req, &resp);

		// Act upon its status.
		if (status.ok()) {
			std::cout << "ResumeReadLogTask recv, ProtocolVersion:" << resp.header().protocolversion() << ", code:" << tapdata::ResponseCode_Name(resp.code()) <<
				", msg:" << resp.msg() << std::endl;
		}
		else {
			std::cout << status.error_code() << ": " << status.error_message()
				<< std::endl;
		}
	}

	void ListReadLogTaskStates()
	{
		ListReadLogTaskStatesResponse resp;

		// Context for the client. It could be used to convey extra information to
		// the server and/or tweak certain RPC behaviors.
		ClientContext context;

		// The actual RPC.
		ListReadLogTaskStatesRequest req;
		req.mutable_header()->set_protocolversion(1);

		Status status = stub_->ListReadLogTaskStates(&context, req, &resp);

		// Act upon its status.
		if (status.ok()) {
			std::cout << "ListReadLogTaskStates recv, ProtocolVersion:" << resp.header().protocolversion() << ", code:" << tapdata::ResponseCode_Name(resp.code()) << ", task:";
			for (const auto& it : resp.taskstate())
			{
				std::cout << "task id:" << it.id() << ", " << "state:" << TaskState_Name(it.state()) << " ";
			}
			std::cout << std::endl;
		}
		else {
			std::cout << status.error_code() << ": " << status.error_message()
				<< std::endl;
		}
	}


	void appendHex(uint64_t value, uint64_t length) {
		uint64_t j = (length - 1) * 4;
		for (uint64_t i = 0; i < length; ++i)
		{
			std::cout << (map16[(value >> j) & 0xF]);
			j -= 4;
		}
	}

	void StartPullReadLog(const std::string& id) {

		ClientContext context;
		//context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(1));

		TaskHandleRequest req;
		req.mutable_header()->set_protocolversion(1);
		req.set_id(id);

		std::unique_ptr<ClientReader<ReadLogResponse> > reader(
			stub_->PullReadLog(&context, req));

		ReadLogResponse resp;
		std::string str;
		int64_t recv_size = 0;
		auto begin = std::chrono::steady_clock::now();

		while (reader->Read(&resp)) {
			++recv_size;
			//std::this_thread::sleep_for(std::chrono::seconds(1));
			//if (recv_size == 1000)
			//	be1 = std::chrono::steady_clock::now();
			//if(recv_size == 4000)
			//	std::cout << "recv package:" << recv_size << ", cost ms:" << std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now() - begin).count() << std::endl;
			//if (recv_size % 1000 == 0)
			//{
				//auto const_ms = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now() - begin).count();
				//std::cout << "recv package:" << recv_size << ", cost ms:" << const_ms << std::endl;
			//}

			if (google::protobuf::TextFormat::PrintToString(resp, &str))
			{
				std::cout << "\n" << str << "\n" << std::endl;
			}

			//if (recv_size >= 1000 && recv_size <= 4000 && google::protobuf::TextFormat::PrintToString(resp, &str))
			//	std::cout << "\n" << str << "\n";

			std::cout << "\n" << recv_size << "\n";

		}
		Status status = reader->Finish();
		if (status.ok()) {
			std::cout << "rpc succeeded." << std::endl;
		}
		else {
			std::cout << "rpc failed." << std::endl;
		}

		auto const_ms = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now() - begin).count();
		std::cout << "recv package:" << recv_size << ", cost ms:" << const_ms << std::endl;
	}

private:
	std::unique_ptr<DB2ReadLogServer::Stub> stub_;
};


void test(DemoClient& client, DB2Veresion ver, int64_t start_time, std::vector<int>&& table_ids, const std::string& scn, const std::string& host, const std::string& port, const std::string& db, const std::string& passwd, bool bigendian, bool cache_lri)
{
	auto tid = std::this_thread::get_id();
	std::stringstream sst;
	sst << tid;

#if 0
	bool stop = false;

	std::vector<std::thread> threads;
	for (int i = 0; i < 10; ++i)
	{
		threads.emplace_back(
			[&client, &stop, &sst, ver, tableid]() {
				while (!stop)
				{
					client.Ping();
					client.ServerInfo();
					if (ver == decltype(ver)::V9)
					{
						client.CreateReadlogTaskV09(sst.str(), tableid);
					}
					else if (ver == decltype(ver)::V10)
					{
						client.CreateReadlogTaskV10(sst.str(), tableid);
					}
					client.DeleteReadLogTask(sst.str());
					client.ResumeReadLogTask(sst.str());
					client.PauseReadLogTask(sst.str());
					client.ListReadLogTaskStates();
					//std::this_thread::sleep_for(std::chrono::milliseconds(1));//
					std::cout << std::endl;
					std::cout << std::endl;
				}
			}
		);
	}

#endif


	if (ver == decltype(ver)::V10)
	{
		auto create_test_re = std::async([&sst, start_time, &table_ids, &client, scn, host, port, db, passwd, bigendian, cache_lri]()
			{
				client.CreateReadlogTaskV10(sst.str(), std::move(table_ids), start_time, scn, host, port, db, passwd, bigendian, cache_lri);
			});

		create_test_re.wait();
	}
	else if (ver == decltype(ver)::V9)
	{
		auto create_test_re = std::async([&sst, start_time, &table_ids, &client, scn]()
			{
				client.CreateReadlogTaskV09(sst.str(), std::move(table_ids), start_time, scn);
			});

		create_test_re.wait();
	}

	std::cout << "create_test_re go" << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(2));
	client.ListReadLogTaskStates();
	auto state_test_re = std::async([&client, &sst]()
		{
			//std::this_thread::sleep_for(std::chrono::seconds(10));
			//client.PauseReadLogTask(sst.str());
			//std::this_thread::sleep_for(std::chrono::seconds(5));
			//client.ResumeReadLogTask(sst.str());
			std::this_thread::sleep_for(std::chrono::seconds(3600));//
			client.DeleteReadLogTask(sst.str());
		});

	int pull_retry_times = 4;
	for (int i = 0; i < pull_retry_times; ++i)
	{
		client.StartPullReadLog(sst.str());
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	//std::this_thread::sleep_for(std::chrono::seconds(3600));
	state_test_re.wait();

#if 0
	stop = true;
	for (auto& i : threads)
		i.join();
#endif
}

void sig_int_proc(int sig_no)
{
	system("kill `ps -ef|grep -w \"db210_src_srv C\"|grep -v grep|awk '{print $2}'`");
	raise(SIGTERM);
}

template<typename T>
void reverse_bytes(T* start)
{
	char* istart = (char*)start, * iend = istart + sizeof(T);
	std::reverse(istart, iend);
}

int main(int argc, char** argv)
{
	signal(SIGINT, sig_int_proc);

	int64_t start_time = 0;
	std::string scn{};
	std::string number{};
	std::string host = "192.168.1.132";
	std::string port = "50000";
	std::string db = "TESTDB";
	std::string passwd = "Gotapd8!"; // Eo36_MCf
	bool bigendian = false;
	bool cache_lri = false;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-reverse") == 0) {
			number = argv[++i];
			std::istringstream tmp(number.c_str());
			unsigned long long num;
	                tmp >> num;
			reverse_bytes(&num);
			std::cout << num << std::endl;
			return 0;
		}

		if (strcmp(argv[i], "-st") == 0)
		{
			try {
				start_time = atoll(argv[++i]);
			}
			catch (...)
			{
				std::cerr << "error, check second param" << std::endl;
			}
		}
		else if (strcmp(argv[i], "-scn") == 0)
		{
			scn = argv[++i];
		}
		else if (strcmp(argv[i], "-host") == 0)
		{
			host = argv[++i];
		}
		else if (strcmp(argv[i], "-port") == 0)
		{
			port = argv[++i];
		}
		else if (strcmp(argv[i], "-db") == 0)
		{
			db = argv[++i];
		}
		else if (strcmp(argv[i], "-passwd") == 0)
		{
			passwd = argv[++i];
		}
		else if (strcmp(argv[i], "-bigendian") == 0)
		{
			char* tmp = argv[++i];
			if (strcmp(tmp, "1") == 0 || strcmp(tmp, "true") == 0) {
				bigendian = true;
			}
		}
		else if (strcmp(argv[i], "-cl") == 0)
		{
			char* tmp = argv[++i];
			if (strcmp(tmp, "1") == 0 || strcmp(tmp, "true") == 0) {
				cache_lri = true;
			}
		}
	}

#if 0
	DemoClient clientv10(
		grpc::CreateChannel("192.168.1.132:50031", grpc::InsecureChannelCredentials()));
#else
	DemoClient clientv10(
		grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials()));
#endif
	DemoClient clientv09(
		grpc::CreateChannel("127.0.0.1:50030", grpc::InsecureChannelCredentials()));

	std::vector<int> table_ids{};
	// table_ids.push_back(4);

	//test(clientv09, DB2Veresion::V9, start_time, std::move(tables));



	test(clientv10, DB2Veresion::V10, start_time, std::move(table_ids), scn, host, port, db, passwd, bigendian, cache_lri);

	return 0;
}
