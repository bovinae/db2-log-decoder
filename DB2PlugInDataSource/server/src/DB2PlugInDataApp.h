#pragma once
#include <thread>
//#include <mutex>
#include <memory>
#include "local_app.h"
#include "GrpcServer.h"
#include "GrpcTypeHeader.h"
#include "ReadLogManager.h"

namespace tapdata
{
    class DB2PlugInDataApp final : public local_app
    {
    public:
        DB2PlugInDataApp();
        ~DB2PlugInDataApp();

    private:
        void initGrpcTasks();

    private://callback func

        grpc::Status pingInvoke(UnaryRpcPing::RpcReqType&&, ServerTask::ServerTaskID task_id,
            UnaryRpcPing::PushTaskResultStreamFuncT push);

        grpc::Status serverInfoInvoke(UnaryRpcServerInfo::RpcReqType&&, ServerTask::ServerTaskID task_id,
            UnaryRpcServerInfo::PushTaskResultStreamFuncT push);


        grpc::Status createReadLogTaskInvoke(UnaryRpcCreateReadLogTask::RpcReqType&&, ServerTask::ServerTaskID task_id,
            UnaryRpcCreateReadLogTask::PushTaskResultStreamFuncT push);

        grpc::Status deleteReadLogTaskInvoke(UnaryRpcDeleteReadLogTask::RpcReqType&&, ServerTask::ServerTaskID task_id,
            UnaryRpcDeleteReadLogTask::PushTaskResultStreamFuncT push);

        grpc::Status pauseReadLogTaskInvoke(UnaryRpcPauseReadLogTask::RpcReqType&&, ServerTask::ServerTaskID task_id,
            UnaryRpcPauseReadLogTask::PushTaskResultStreamFuncT push);

        grpc::Status resumeReadLogTaskInvoke(UnaryRpcResumeReadLogTask::RpcReqType&&, ServerTask::ServerTaskID task_id,
            UnaryRpcResumeReadLogTask::PushTaskResultStreamFuncT push);


        grpc::Status listReadLogTaskStatesInvoke(UnaryRpcListReadLogTaskStates::RpcReqType&&, ServerTask::ServerTaskID task_id,
            UnaryRpcListReadLogTaskStates::PushTaskResultStreamFuncT push);

        grpc::Status getReadLogTaskStateInvoke(UnaryRpcGetReadLogTaskState::RpcReqType&&, ServerTask::ServerTaskID task_id,
            UnaryRpcGetReadLogTaskState::PushTaskResultStreamFuncT push);

        grpc::Status pullReadLogInvoke(const StreamToClient::RpcReqType&, ServerTask::ServerTaskID, StreamToClient::TaskResultT&);

        grpc::Status pushReadLogInvoke(UnaryPushReadLogTask::RpcReqType&&, ServerTask::ServerTaskID task_id,
            UnaryPushReadLogTask::PushTaskResultStreamFuncT push);

        void pullReadLogFinished(const ServerTask::ServerTaskID&);

    protected:
        void run() noexcept override;
        void sig_term_proc(int sig_no) override;
        void sig_crash_proc(int sig_no) override;
        void sig_dump_proc(int sig_no) override;

    private:
        void thread_func() noexcept;

    private:
        bool keep_run_{ false };
        std::thread thread_;
        std::unique_ptr<ReadLogManager> read_log_manager_;
        std::unique_ptr<GrpcServer> grpc_server_;
    };
}