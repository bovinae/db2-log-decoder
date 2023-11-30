#pragma once
#include "GrpcTypeHeader.h"
#include "StreamToClient.h"

namespace tapdata
{
    class readlog_config_manager;
    class ReadLogManager
    {
    public:
        ReadLogManager();
        ~ReadLogManager();

    public:
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

        void pullReadLogFinished(const ServerTask::ServerTaskID&);


        grpc::Status pushReadLogInvoke(UnaryPushReadLogTask::RpcReqType&&, ServerTask::ServerTaskID task_id,
            UnaryPushReadLogTask::PushTaskResultStreamFuncT push);


    private:
        readlog_config_manager* readlog_config_manager_{};
    };

}