#pragma once
#include <memory>
#include <functional>
#include <utility>
#include <cstdint>
#include <exception>
#include <atomic>

#include <grpcpp/grpcpp.h>
#include "DB2PlugInDataSource.grpc.pb.h"
#include "log_imp.h"


#include "ServerTask.h"
#include "blocking_queue.h"

#ifdef SERVER_DEBUG
#include "log_imp.h"
#include <google/protobuf/text_format.h>
#endif

namespace tapdata
{
    class StreamToClient final :public ServerTask
    {
    public:
        using RpcServerType = DB2ReadLogServer::AsyncService;
        using RpcReqType = TaskHandleRequest;
        using RpcRespType = ReadLogResponse;

        using TaskResultInfo = std::unique_ptr<RpcRespType>;
        using TaskResultT = std::unique_ptr<std::pair<ServerTaskID, TaskResultInfo>>;
        using PullTaskResultFuncT = std::function<grpc::Status(const RpcReqType&, ServerTaskID, TaskResultT&)>;
        using FinishMethodFuncT = std::function<void(const ServerTaskID&)>;

        StreamToClient
        (
            RpcServerType* service, grpc::ServerCompletionQueue* cq,
            PullTaskResultFuncT invoke_method, FinishMethodFuncT finish_method, std::uint32_t loop_wait_ms) :
            ServerTask(),
            service_(service), cq_(cq), ctx_(new grpc::ServerContext),
            prepare_stop_(false), stop_(false),
            request_done_func_(std::bind(&StreamToClient::request_done, this, std::placeholders::_1)),
            write_done_func_(std::bind(&StreamToClient::write_done, this, std::placeholders::_1)),
            finish_done_func_(std::bind(&StreamToClient::finish_done, this, std::placeholders::_1)),
            invoke_method_(invoke_method), finish_method_(finish_method),
            loop_wait_ms_(loop_wait_ms),
            resp_stream_(ctx_.get())
        {
            thread_ = std::thread(&StreamToClient::thread_run, this);
        }

        void prepare_stop() override
        {
            prepare_stop_ = true;
            stop_ = true;
            if (thread_.joinable())
                thread_.join();
        }

        ~StreamToClient()
        {
            prepare_stop();
        }

    private:
        void Reset()
        {
            if (!prepare_stop_)
            {
                last_fail_ = false;
                ctx_.reset(new grpc::ServerContext);
                resp_stream_ = grpc::ServerAsyncWriter<RpcRespType>(ctx_.get());
                service_->RequestPullReadLog(ctx_.get(), &req_, &resp_stream_, cq_, cq_, &request_done_func_);
            }
        }

        void thread_run()
        {
            service_->RequestPullReadLog(ctx_.get(), &req_, &resp_stream_, cq_, cq_, &request_done_func_);
            decltype(states_.take()) state;
            while (!stop_)
            {
                if (!states_.try_wait_and_pop(state, loop_wait_ms_))
                    continue;
                if (state.first == decltype(state.first)::RequestDone)
                {
                    if (!request(state.second)) {
                        LOG_DEBUG("Reset service_->RequestPullReadLog");
                        Reset();
                    }
                }
                else if (state.first == decltype(state.first)::WriteDone)
                {
                    if (!write(state.second)) {
                        LOG_DEBUG("Reset service_->RequestPullReadLog");
                        Reset();
                    }
                }
                else if (state.first == decltype(state.first)::FinishDone)
                {
                    if (!finish(state.second)) {
                        // LOG_DEBUG("Reset service_->RequestPullReadLog");
                        Reset();
                    }
                }
                else
                {
                    throw std::runtime_error("state logic error, state first is:" + std::to_string(static_cast<int>(state.first)));
                }
            }
        }

        bool request(bool ok)
        {
            // LOG_DEBUG("request, ok:{}", ok);
            if (!ok) {
                return false;
            }
            // invoke the method
            // Call the RPC processing function
            ++task_id_;

#ifdef SERVER_DEBUG
            std::string debug_temp;
            if (google::protobuf::TextFormat::PrintToString(req_, &debug_temp))
            {
                LOG_INFO("recv task_id:{}, pullReadLogInvoke:{}", task_id_, debug_temp);
            }
#endif // SERVER_DEBUG

            TaskResultT results;
            grpc::Status status = invoke_method_(req_, task_id_, results);

            if (!status.ok())
            {
                resp_stream_.Finish(status, &finish_done_func_);
                return true;
            }

            if (!results->second)
            {
                resp_stream_.Finish(grpc::Status::OK, &finish_done_func_);
            }
            else
            {
#ifdef SERVER_DEBUG
                std::string debug_temp;
                if (google::protobuf::TextFormat::PrintToString(*results->second, &debug_temp))
                {
                    LOG_INFO("back :{}", debug_temp);
                }
#endif // SERVER_DEBUG

                resp_stream_.Write(*results->second, &write_done_func_);
                if (results->second->code() != decltype(results->second->code())::OK)
                    last_fail_ = true;
            }
            return true;
        }

        bool write(bool ok)
        {
            // LOG_DEBUG("write, ok:{}", ok);
            if (ok) {
                if (last_fail_)
                    resp_stream_.Finish(grpc::Status::OK, &finish_done_func_);
                else
                {
                    TaskResultT results;
                    grpc::Status status = invoke_method_(req_, task_id_, results);

                    if (!status.ok())
                    {
                        resp_stream_.Finish(status, &finish_done_func_);
                        return true;
                    }

                    if (!results->second)
                    {
                        resp_stream_.Finish(grpc::Status::OK, &finish_done_func_);
                    }
                    else
                    {
#ifdef SERVER_DEBUG
                        std::string debug_temp;
                        if (google::protobuf::TextFormat::PrintToString(*results->second, &debug_temp))
                        {
                            LOG_INFO("back :{}", debug_temp);
                        }
#endif // SERVER_DEBUG
                        resp_stream_.Write(*results->second, &write_done_func_);
                        if (results->second->code() != decltype(results->second->code())::OK)
                            last_fail_ = true;
                    }
                }
            }
            else {  // must be done so let's finish
                resp_stream_.Finish(grpc::Status::OK, &finish_done_func_);
            }
            return true;
        }

        bool finish(bool /*ok*/)
        {
            // LOG_DEBUG("finish");
            finish_method_(task_id_);
            return false;
        }

        bool request_done(bool ok)
        {
            // LOG_DEBUG("request_done, ok:{}", ok);
            states_.put(std::make_pair(ServerTaskState::RequestDone, ok));
            return true;
        }

        bool write_done(bool ok)
        {
            // LOG_DEBUG("write_done, ok:{}", ok);
            states_.put(std::make_pair(ServerTaskState::WriteDone, ok));
            return true;
        }

        bool finish_done(bool ok)
        {
            // LOG_DEBUG("finish_done, ok:{}", ok);
            states_.put(std::make_pair(ServerTaskState::FinishDone, ok));
            return false;
        }

    private:
        RpcServerType* service_;
        grpc::ServerCompletionQueue* cq_;

        std::unique_ptr<grpc::ServerContext> ctx_;

        ServerTaskID task_id_{};
        std::atomic<bool> prepare_stop_;
        std::atomic<bool> stop_;
        bool last_fail_ = false;
        ChangeServerTaskFuncT request_done_func_;
        ChangeServerTaskFuncT write_done_func_;
        ChangeServerTaskFuncT finish_done_func_;

        PullTaskResultFuncT invoke_method_;
        FinishMethodFuncT finish_method_;

        blocking_queue<std::pair<ServerTaskState, bool>> states_;

        const uint32_t loop_wait_ms_;
        TaskHandleRequest req_;
        grpc::ServerAsyncWriter<RpcRespType> resp_stream_;


        std::thread thread_;
    };
}

