#pragma once
#include <memory>
#include <functional>
#include <utility>
#include <cstdint>
#include <exception>
#include <atomic>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "ServerTask.h"
#include "blocking_queue.h"

#ifdef SERVER_DEBUG
#include "log_imp.h"
#include <google/protobuf/text_format.h>
#endif

namespace tapdata
{
    template<typename ServerType, typename ReqType, typename RespType>
    class UnaryRpc final :public ServerTask
    {
    public:
        using RpcServerType = ServerType;
        using RpcReqType = ReqType;
        using RpcRespType = RespType;
        using RequestFunc = void (RpcServerType::*)(grpc::ServerContext*, RpcReqType*,
            grpc::ServerAsyncResponseWriter<RpcRespType>*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void*);
        using TaskResultInfo = std::unique_ptr<RpcRespType>;
        using TaskResultT = std::unique_ptr<std::pair<ServerTaskID, TaskResultInfo>>;
        using PushTaskResultStreamFuncT = std::function<void(TaskResultT&& result)>;
        using InvokeMethodFuncT = std::function<grpc::Status(RpcReqType&&, ServerTaskID, PushTaskResultStreamFuncT)>;

        UnaryRpc
        (
            RpcServerType* service, grpc::ServerCompletionQueue* cq, RequestFunc req_func, /*RpcRespType&& default_finish_resp,*/
            InvokeMethodFuncT invoke_method, std::uint32_t loop_wait_ms) :
            ServerTask(),
            service_(service), cq_(cq), req_func_(req_func),
            /*def_resp_(std::move(default_finish_resp)),*/ ctx_(new grpc::ServerContext),
            prepare_stop_(false), stop_(false),
            request_done_func_(std::bind(&UnaryRpc::request_done, this, std::placeholders::_1)),
            finish_done_func_(std::bind(&UnaryRpc::finish_done, this, std::placeholders::_1)),
            invoke_method_(invoke_method),
            loop_wait_ms_(loop_wait_ms),
            resp_writer_(ctx_.get())
        {
            thread_ = std::thread(&UnaryRpc::thread_run, this);
        }

        void prepare_stop() override
        {
            bool lastp = prepare_stop_;
            prepare_stop_ = true;
            stop_ = true;

            if (!lastp)
            {
                TaskResultT status(new typename TaskResultT::element_type);//notify resp_
                status->first = task_id_;
                resp_.put(std::move(status));
            }

            if (thread_.joinable())
                thread_.join();
        }

        ~UnaryRpc()
        {
            prepare_stop();
        }

    private:
        void Reset()
        {
            if (!prepare_stop_)
            {
                ctx_.reset(new grpc::ServerContext);
                resp_writer_ = grpc::ServerAsyncResponseWriter<RpcRespType>(ctx_.get());
                (service_->*req_func_)(ctx_.get(), &req_, &resp_writer_, cq_, cq_, &request_done_func_);
            }
        }

        void thread_run()
        {
            (service_->*req_func_)(ctx_.get(), &req_, &resp_writer_, cq_, cq_, &request_done_func_);
            decltype(states_.take()) state;
            while (!stop_)
            {
                if (!states_.try_wait_and_pop(state, loop_wait_ms_))
                    continue;
                if (state.first == decltype(state.first)::RequestDone)
                {
                    if (!request(state.second))
                        Reset();
                }
                else if (state.first == decltype(state.first)::FinishDone)
                {
                    if (!finish(state.second))
                        Reset();
                }
                else
                {
                    throw std::runtime_error("state logic error, state first is:" + std::to_string(static_cast<int>(state.first)));
                }
            }
        }

        void insert(TaskResultT&& status)
        {
            resp_.put(std::move(status));
        }

        bool request(bool ok)
        {
            if (!ok) {
                return false;
            }

            ++task_id_;
            grpc::Status status = invoke_method_(std::move(req_), task_id_, std::bind(&UnaryRpc::insert, this, std::placeholders::_1));

            if (!status.ok())
            {
                //resp_writer_.Finish(def_resp_, status, &finish_done_func_);
                resp_writer_.Finish({}, status, &finish_done_func_);
                return true;
            }

            while (true)
            {
                auto&& tr = resp_.take();
                if (tr->first != task_id_)
                    continue;
                if (!tr->second)
                {
                    resp_writer_.Finish({}, grpc::Status(grpc::StatusCode::UNKNOWN, "stop by server"), &finish_done_func_);
                    break;
                }
                else
                {
#ifdef SERVER_DEBUG
                    std::string debug_temp;
                    if (google::protobuf::TextFormat::PrintToString(*tr->second, &debug_temp))
                    {
                        LOG_INFO("back :{}", debug_temp);
                    }
#endif // SERVER_DEBUG
                    resp_writer_.Finish(*tr->second, grpc::Status::OK, &finish_done_func_);
                    break;
                }
            }
            return true;
        }

        bool finish(bool /*ok*/)
        {
            return false;
        }

        bool request_done(bool ok)
        {
            states_.put(std::make_pair(ServerTaskState::RequestDone, ok));
            return true;
        }

        bool write_done(bool ok)
        {
            states_.put(std::make_pair(ServerTaskState::WriteDone, ok));
            return true;
        }

        bool finish_done(bool ok)
        {
            states_.put(std::make_pair(ServerTaskState::FinishDone, ok));
            return false;
        }

    private:
        RpcServerType* service_;
        grpc::ServerCompletionQueue* cq_;
        RequestFunc req_func_;
        //const RpcRespType def_resp_;

        std::unique_ptr<grpc::ServerContext> ctx_;
        ServerTaskID task_id_{};
        std::atomic<bool> prepare_stop_;
        std::atomic<bool> stop_;
        ChangeServerTaskFuncT request_done_func_;
        ChangeServerTaskFuncT finish_done_func_;

        InvokeMethodFuncT invoke_method_;

        blocking_queue<std::pair<ServerTaskState, bool>> states_;
        const uint32_t loop_wait_ms_;
        blocking_queue<TaskResultT> resp_;
        RpcReqType req_;
        grpc::ServerAsyncResponseWriter<RpcRespType> resp_writer_;

        std::thread thread_;
    };
}
