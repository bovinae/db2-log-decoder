#pragma once
#include "DB2PlugInDataSource.grpc.pb.h"
#include "UnaryRpc.h"
namespace tapdata
{
    using UnaryRpcPing = UnaryRpc<DB2ReadLogServer::AsyncService, PingRequest, PingResponse>;
    using UnaryRpcServerInfo = UnaryRpc<DB2ReadLogServer::AsyncService, google::protobuf::Empty, ServerInfoResponse>;

    using UnaryRpcCreateReadLogTask = UnaryRpc<DB2ReadLogServer::AsyncService, ReadLogRequest, ControlResponse>;
    using UnaryRpcDeleteReadLogTask = UnaryRpc<DB2ReadLogServer::AsyncService, TaskHandleRequest, ControlResponse>;
    using UnaryRpcPauseReadLogTask = UnaryRpc<DB2ReadLogServer::AsyncService, TaskHandleRequest, ControlResponse>;
    using UnaryRpcResumeReadLogTask = UnaryRpc<DB2ReadLogServer::AsyncService, TaskHandleRequest, ControlResponse>;

    using UnaryPushReadLogTask = UnaryRpc<DB2ReadLogServer::AsyncService, PushReadLogRequest, PushReadLogResponse>;

    using UnaryRpcListReadLogTaskStates = UnaryRpc<DB2ReadLogServer::AsyncService, ListReadLogTaskStatesRequest, ListReadLogTaskStatesResponse>;
    using UnaryRpcGetReadLogTaskState = UnaryRpc<DB2ReadLogServer::AsyncService, TaskHandleRequest, GetReadLogTaskStateResponse>;
}