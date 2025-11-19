#pragma once
#include <cstdint>
#include <string>
#include <iterator>
#include <grpcpp/channel.h>
#include <algorithm>
#include <vector>
#include <future>
#include <atomic>

#include "DB2PlugInDataSource.grpc.pb.h"
#include "db2_log_version.h"
#include "DB2ReadLogApp.h"
#include "log_imp.h"
#include "LocalDDLInfo.h"
#include "batch_queue.h"


namespace tapdata
{
	class PushReadLogClient {
		struct ReadLogPayloadCache
		{
			bool reorgPending{ false };
			std::string transactionId;
			std::vector<ReadLogPayload> payloads;
		};

		struct WrapPushReadLogRequest
		{
			WrapPushReadLogRequest(const ReadLogRequest& req, std::shared_ptr<grpc::Channel> channel) :
				last_result(0), readLogRequest_(req), stub_(DB2ReadLogServer::NewStub(channel))
			{
				req_.set_id(readLogRequest_.id());
				req_.mutable_logresponse()->set_code(tapdata::ResponseCode::OK);
				req_.mutable_logresponse()->set_payloadversion(std::to_string(static_cast<int>(db2_log_payload_version::V1)));

				back_fut = std::async(std::launch::async, [this]() {
					std::deque<ReadLogPayload> payloads;
					while (1)
					{
						batch_payloads_.try_wait_and_pop(payloads, 100, batch_size);
						if (!payloads.empty())
						{
							req_.mutable_logresponse()->mutable_payload()->Clear();
							for (auto& it : payloads)
								req_.mutable_logresponse()->mutable_payload()->Add(std::move(it));

							last_result = push(stub_, resp_, req_);
							if (last_result < 0) 
							{
								keep_run_ = false;
								break;
							}
								
						} else if (!keep_run_) {
							break;
						}
					}
					});
			}

			~WrapPushReadLogRequest()
			{
				keep_run_ = false;
				back_fut.wait();
			}

			int32_t add(std::vector<ReadLogPayload>&& payloads)
			{
				if (last_result < 0)
					return -1;
				while (batch_payloads_.size() > cache_batch_size && get_app<DB2ReadLogApp>()->keep_run()) {
					LOG_DEBUG("batch queue is full");
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
				}
				if (get_app<DB2ReadLogApp>()->keep_run())
				{
					batch_payloads_.put(payloads.begin(), payloads.end());
					return 0;
				}
				return -1;
			}

		private:
			//返回0表示成功，-1表示失败需退出，不会大于0
			int32_t push(std::unique_ptr<DB2ReadLogServer::Stub>& stub_, PushReadLogResponse& resp_, const PushReadLogRequest& req_)
			{
				int32_t result{};
				do {
					while (result > 0)
					{
						if (!get_app<DB2ReadLogApp>()->keep_run())
							break;
						LOG_DEBUG("push sleep ms:{}", result > 0x400 ? 0x400 : result);
						std::this_thread::sleep_for(std::chrono::milliseconds(result > 0x400 ? 0x400 : result));
						result -= 0x400;
					}

					result = (get_app<DB2ReadLogApp>()->keep_run()) ? try_push(stub_, resp_, req_) : -1;
				} while (result > 0);
				return result;
			}

			//-1表示发送停止，0表示发送成功，大于0表示需要下次等待多少毫秒再重新发送
			int32_t try_push(std::unique_ptr<DB2ReadLogServer::Stub>& stub_, PushReadLogResponse& resp_, const PushReadLogRequest& req_)
			{
				grpc::ClientContext context;

				// for (int i = 0; i < req_.logresponse().payload_size(); i++)
				// {
				// 	LOG_DEBUG("PushReadLog through grpc, index:{}, scn:{}", i, req_.logresponse().payload(i).scn());
				// }
				const grpc::Status status = stub_->PushReadLog(&context, req_, &resp_);

				if (!status.ok())
				{
					LOG_ERROR("push message fail, code:{}", status.error_code());
					return -1;
				}

				if (resp_.code() == decltype(resp_.code())::PUSH_OK)
					return 0;
				else if (resp_.code() == decltype(resp_.code())::PUSH_PAUSED)
					return resp_.waittimems() >= 0 ? resp_.waittimems() : -1;
				else if (resp_.code() == decltype(resp_.code())::PUSH_STOP)
				{
					LOG_INFO("push message return PUSH_STOP, code:{}", resp_.code());
					return -1;
				}
				else
				{
					LOG_ERROR("logic error, code not handle, code:{}", resp_.code());
					return -1;
				}
			}

			enum { batch_size = 512, cache_batch_size = batch_size << 8 };
			std::atomic<int32_t> last_result;
			bool keep_run_ = true;
			const ReadLogRequest& readLogRequest_;
			std::unique_ptr<DB2ReadLogServer::Stub> stub_;
			//std::vector<PushReadLogRequest> requests_;
			batch_queue<ReadLogPayload> batch_payloads_;
			PushReadLogRequest req_;
			std::future<void> back_fut;
			PushReadLogResponse resp_;
		};

	public:
		PushReadLogClient(const ReadLogRequest& readLogRequest, std::shared_ptr<grpc::Channel> channel) :
			readLogRequest_{ readLogRequest }, wreq_(readLogRequest, channel)
		{

		}

		//0为ok，小于0出错
		int32_t push_dml(ReadLogPayload&& payload, bool reorgPending)
		{
			return append_dml_payload(std::move(payload), reorgPending);
		}

		//0为ok，小于0出错
		int32_t push_commit(ReadLogPayload&& payload)
		{
			return append_commit_payload(std::move(payload));
		}

		//0为ok，小于0出错
		int32_t push_heartbeat(ReadLogPayload&& payload)
		{			
			const auto it = std::find_if(readLogPayloadCaches_.begin(), readLogPayloadCaches_.end(),
				[&payload](const ReadLogPayloadCache& i) { return i.transactionId == payload.transactionid(); });

			if (it != readLogPayloadCaches_.end())//找到了就修改
			{
				it->payloads.emplace_back(std::move(payload));
			}
			else
			{
				//没找到就新增
				auto transactionId = payload.transactionid();
				std::vector<ReadLogPayload> payloads{ std::move(payload) };
				readLogPayloadCaches_.emplace_back(ReadLogPayloadCache{ false, std::move(transactionId), std::move(payloads) });
			}
			return flash_push();
		}

		//0为ok，小于0出错
		int32_t push_abort(ReadLogPayload&& payload)
		{
			//LOG_DEBUG("recv abort log: transaction id:{}", payload.transactionid());
			const auto it = std::find_if(readLogPayloadCaches_.begin(), readLogPayloadCaches_.end(),
				[&payload](const ReadLogPayloadCache& i) { return i.transactionId == payload.transactionid(); });

			if (it != readLogPayloadCaches_.end())//找到了就删除
				readLogPayloadCaches_.erase(it);

			return 0;
		}

		//0为ok，小于0出错
		int32_t push_ddl(ReadLogPayload&& payload, LocalDDLInfo&& info)
		{
			return append_ddl_payload(std::move(payload), std::move(info));
		}

		//0为ok，小于0出错
		int32_t push_undo_ddl(ReadLogPayload&& payload)
		{
			LOG_DEBUG("recv undo ddl log: transaction id:{}", payload.transactionid());
			const auto it = std::find_if(readLogPayloadCaches_.begin(), readLogPayloadCaches_.end(),
				[&payload](const ReadLogPayloadCache& i) { return i.transactionId == payload.transactionid(); });
			if (it != readLogPayloadCaches_.end())//找到了就删除
			{
				const auto temp_it = std::remove_if(it->payloads.begin(), it->payloads.end(),
					[&payload](const ReadLogPayload& p) { return p.op() == decltype(p.op())::DDL; });
				it->payloads.erase(temp_it, it->payloads.end());
			}
			return 0;
		}

		//0为ok，小于0出错
		int32_t push_undo_dml(ReadLogPayload&& payload)
		{
			// LOG_DEBUG("recv undo dml log: transaction id:{}", payload.transactionid());
			const auto it = std::find_if(readLogPayloadCaches_.begin(), readLogPayloadCaches_.end(),
				[&payload](const ReadLogPayloadCache& i) { return i.transactionId == payload.transactionid(); });
			if (it != readLogPayloadCaches_.end())//找到了就删除
			{
				const auto temp_it = std::remove_if(it->payloads.begin(), it->payloads.end(),
					[payload](const ReadLogPayload& p) { return p.rid() == payload.rid(); });
				it->payloads.erase(temp_it, it->payloads.end());
			}
			return append_dml_payload(std::move(payload), false);//undo也属于 dml一种
		}

		//0为ok，小于0出错
		int32_t push_reorg_table(ReadLogPayload&& payload)
		{
			LOG_DEBUG("recv reorg table log: transaction id:{}", payload.transactionid());
			for (auto it = readLogPayloadCaches_.begin();; ++it)//扫描所有的payload，只要是tableid与reorg table消息相同，则取消暂存状态
			{
				it = std::find_if(it, readLogPayloadCaches_.end(),
					[&payload](const ReadLogPayloadCache& i)
					{
						if (!i.reorgPending) {
							LOG_DEBUG("i.reorgPending is false");
							return false;
						}
						auto tmp = std::any_of(i.payloads.cbegin(), i.payloads.cend(),
							[&payload](const ReadLogPayload& p) {return p.tableid() == payload.tableid(); });
						LOG_DEBUG("not found equal tableid 1");
						return tmp;
					}
				);

				if (it == readLogPayloadCaches_.end()) {
					LOG_DEBUG("not found equal tableid 2");
					break;
				}

				LOG_DEBUG("transaction:{} reorg pending change false now", it->transactionId);
				it->reorgPending = false;//取消暂存状态
			}

			LOG_DEBUG("insert empty to trigger append_commit_payload");
			readLogPayloadCaches_.emplace_back(ReadLogPayloadCache{ false, payload.transactionid(), {} });//插入空的为了触发append_commit_payload提交
			return 0;
		}

		int32_t flash_push(bool flush_all = false)
		{
			int32_t result{};

			auto push_begin = readLogPayloadCaches_.begin();
			if (!flush_all)
			{
				push_begin = std::stable_partition(readLogPayloadCaches_.begin(), readLogPayloadCaches_.end(),
				[](const ReadLogPayloadCache& i) {return !(!i.reorgPending && !i.payloads.empty() && (i.payloads.back().op() == decltype(i.payloads.back().op())::COMMIT || i.payloads.back().op() == decltype(i.payloads.back().op())::HEARTBEAT)); });	
			}

			for (auto it = push_begin; it != readLogPayloadCaches_.end(); ++it)
			{
				result = push_paylods(std::move(it->payloads));
				if (result != 0)
					break;
			}

			readLogPayloadCaches_.erase(push_begin, readLogPayloadCaches_.end());//清空掉已经标记发送状态的缓存

			return result;
		}

	private:
		//返回0表示成功，-1表示失败需退出
		int32_t push_paylods(std::vector<ReadLogPayload>&& payloads)
		{
			return wreq_.add(std::move(payloads));
		}

		bool test_dml_needed(const ReadLogPayload& payload)
		{
			if (test_drop_first_scn)
			{
				if (payload.scn() == readLogRequest_.scn()) { //表示断点设定了scn是需要丢弃的dml
					LOG_DEBUG("not drop first scn, scn:{}", payload.scn());
					return true;
				} else
					test_drop_first_scn = false;//表示第一条scn已经丢弃完毕，下次不需要再进入此判断
			}
			return readLogRequest_.tables().empty() || std::any_of(readLogRequest_.tables().cbegin(), readLogRequest_.tables().cend(),
				[&payload](const SourceTable& table) {return table.tableid() == payload.tableid(); });//过滤列表为空或者列表是所需table，则是需要的dml
		}

		bool is_special_update(const ReadLogPayload& payload)
		{
			if (payload.beforelogbytes().length() == 4 && payload.logbytes().length() == 4) {
				return true;

				char before[4]{0, 6, 0, 0};
				for(size_t i = 0; i < payload.beforelogbytes().length(); i++) {
					char tmp1 = payload.beforelogbytes().data()[i];
					if (before[i] != tmp1) return false;
				}
				char after[4]{0, 0, 0, 0};
				for(size_t i = 0; i < payload.logbytes().length(); i++) {
					char tmp1 = payload.logbytes().data()[i];
					if (after[i] != tmp1) return false;
				}
				return true;
			}
			return false;
		}

		//dml带有tableid
		int32_t append_dml_payload(ReadLogPayload&& payload, bool reorgPending)
		{
			//过滤掉不需要的数据
			auto isNeeded = test_dml_needed(payload);

			if (!isNeeded && payload.op() != decltype(payload.op())::UNKNOWN) { //既不需要也不是控制消息，直接返回，丢弃该payload，UNKNOWN代表控制消息
				//LOG_DEBUG("not append dml, scn:{}, tid:{}", payload.scn(), payload.transactionid());
				return 0;
			}

			const auto it = std::find_if(readLogPayloadCaches_.begin(), readLogPayloadCaches_.end(),
				[&payload](const ReadLogPayloadCache& i) { return i.transactionId == payload.transactionid(); });

			if (isNeeded)//需要的消息
			{
				if (it != readLogPayloadCaches_.end())//缓存里可以找到这个事务
				{
					if (reorgPending)//需要暂存重组
					{
						it->reorgPending = reorgPending;//设置整个事务需要暂存
						for (auto& p : it->payloads)//暂存消息要为之前的DDL关联tableid
						{
							if (p.op() == decltype(p.op())::DDL && p.tableid() == 0)
							{
								p.set_tableid(payload.tableid());
								p.set_tablespaceid(payload.tablespaceid());
							}
						}
					}

					if (payload.op() != decltype(payload.op())::UNKNOWN) {//UNKNOWN代表控制消息，不放入payload
						if (payload.op() == decltype(payload.op())::UPDATE && is_special_update(payload) && it->payloads.back().op() == decltype(payload.op())::DELETE) {
							LOG_DEBUG("drop special update, last payload scn:{}, payload scn: {}", it->payloads.back().scn(), payload.scn());
							it->payloads.pop_back();
						} else {
                            //LOG_DEBUG("append paload, scn:{}", payload.scn());
							it->payloads.emplace_back(std::move(payload));
						}
					}
				}
				else//没找到就新增
				{
					auto transactionId = payload.transactionid();
					std::vector<ReadLogPayload> payloads;
					if (payload.op() != decltype(payload.op())::UNKNOWN) { //UNKNOWN代表控制消息，不放入payload
						//LOG_DEBUG("append dml, scn:{}, tid:{}", payload.scn(), payload.transactionid());
						payloads.emplace_back(std::move(payload));
					}
					readLogPayloadCaches_.emplace_back(ReadLogPayloadCache{ reorgPending, std::move(transactionId), std::move(payloads) });
				}
			}
			else//不需要的信息
			{
				if (it != readLogPayloadCaches_.end())//把之前的缓存消息符合该tableid的全部清空，包括ddl
				{
					LOG_DEBUG("remove message, scn:{}, tid:{}", payload.scn(), payload.transactionid());
					const auto temp_it = std::remove_if(it->payloads.begin(), it->payloads.end(),
						[&payload](const ReadLogPayload& p) { return p.tableid() == payload.tableid() || p.op() == decltype(p.op())::DDL; });
					it->payloads.erase(temp_it, it->payloads.end());
				}
			}

			return 0;
		}

		int32_t append_commit_payload(ReadLogPayload&& payload)
		{
			// LOG_DEBUG("commit transactiontime:{},scn:{}", payload.transactiontime(), payload.scn());
			const auto it = std::find_if(readLogPayloadCaches_.begin(), readLogPayloadCaches_.end(),
				[&payload](const ReadLogPayloadCache& i) { return i.transactionId == payload.transactionid(); });
			if (it == readLogPayloadCaches_.end())
				return 0;

			if (it->payloads.empty() || (readLogRequest_.stime() > 0 && payload.transactiontime() < readLogRequest_.stime()))//空的或者在所需时间戳之前的就删除
				readLogPayloadCaches_.erase(it);
			else
			{
				sqluint32 transactionTime = payload.transactiontime();
				for (auto &i : it->payloads) {
					i.set_transactiontime(transactionTime);
				}
				it->payloads.emplace_back(std::move(payload));
				if (it->reorgPending) { //暂存的就返回
					LOG_DEBUG("reorgPending, scn:{}", payload.scn());
					return 0;
				}
			}
			return flash_push();
		}

		//ddl的tableid总是0
		int32_t append_ddl_payload(ReadLogPayload&& payload, LocalDDLInfo&& info)
		{
			//如果是可识别的ddl，先检测在别的暂存事务里是否有相同表的ddl
			bool needReorgPending = false;
			if (!info.schema.empty() && !info.tableName.empty() && (info.option == "RENAME" || info.option == "COMMENT"))
			{
				for (const auto& it : readLogPayloadCaches_)//先找在暂存状态里的ddl是否存在
				{
					if (!it.reorgPending)//不需要暂存就跳过
						continue;
					const auto i = std::find_if(it.payloads.cbegin(), it.payloads.cend(),
						[&payload](const ReadLogPayload& p) {return p.schema() == payload.schema() && p.tablename() == payload.tablename(); });
					if (i != it.payloads.cend())
					{
						needReorgPending = true;
						payload.set_tableid(i->tableid());//让ddl关联table id，后续reorg消息才可以解除暂存状态
						payload.set_tablespaceid(i->tablespaceid());
						break;
					}
				}
			}

			const auto it = std::find_if(readLogPayloadCaches_.begin(), readLogPayloadCaches_.end(),
				[&payload](const ReadLogPayloadCache& i) { return i.transactionId == payload.transactionid(); });

			if (it != readLogPayloadCaches_.end())//找到了就修改
			{
				it->payloads.emplace_back(std::move(payload));
			}
			else
			{
				//没找到就新增
				auto transactionId = payload.transactionid();
				std::vector<ReadLogPayload> payloads{ std::move(payload) };
				readLogPayloadCaches_.emplace_back(ReadLogPayloadCache{ needReorgPending, std::move(transactionId), std::move(payloads) });
			}
			return 0;
		}




	private:
		bool test_drop_first_scn = true;
		const ReadLogRequest& readLogRequest_;
		std::vector<ReadLogPayloadCache> readLogPayloadCaches_;
		WrapPushReadLogRequest wreq_;
	};
}
