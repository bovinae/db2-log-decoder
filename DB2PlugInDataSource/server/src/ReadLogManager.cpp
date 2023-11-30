#include <limits.h>
#include <cassert>
#include "ReadLogManager.h"
#include "tool.h"
#include "log_imp.h"
#include "DB2PlugInDataApp.h"
#include "readlog_config_manager.h"
#include "db2_log_version.h"

#ifdef SERVER_DEBUG
#include <google/protobuf/text_format.h>
#endif

namespace tapdata
{
    using namespace std;
#ifdef SERVER_DEBUG
    using google::protobuf::TextFormat;
#endif
    constexpr size_t SQL_DBNAME_SZ = 8;
    constexpr size_t USERID_SZ = 128;
    constexpr size_t PSWD_SZ = 14;
    constexpr size_t SQL_HOSTNAME_SZ = 255;
    constexpr size_t SQL_SERVICE_NAME_SZ = 14;

    ResponseCode parse(UnaryRpcCreateReadLogTask::RpcReqType&& req, shared_ptr<readlog_config>& config)
    {
        tool::trim(req.mutable_source()->mutable_databasehostname()[0]);
        tool::trim(req.mutable_source()->mutable_databaseservicename()[0]);
        tool::trim(req.mutable_source()->mutable_databasename()[0]);
        tool::trim(req.mutable_source()->mutable_databaseusername()[0]);

        if (req.target().type() != decltype(req.target().type())::GRPC)
            return ResponseCode::NOT_SUPPORT;

        if (req.source().databaseversion() != static_cast<DB2Veresion>(db2_log_version::AUTO) &&
            req.source().databaseversion() != static_cast<DB2Veresion>(LocalDB2LogVersion))
            return ResponseCode::NOT_SUPPORT;

        if (req.source().databasename().empty() || 
            req.source().databaseusername().empty() || req.source().databasepassword().empty() ||
            req.source().databasehostname().empty() || req.source().databaseservicename().empty()
            )
            return ResponseCode::INVALID_PARAM;

        if (req.source().databasename().size() > SQL_DBNAME_SZ ||
            req.source().databaseusername().size() > USERID_SZ || req.source().databasepassword().size() > PSWD_SZ ||
            req.source().databasehostname().size() > SQL_HOSTNAME_SZ || req.source().databaseservicename().size() > SQL_SERVICE_NAME_SZ
            )
            return ResponseCode::INVALID_PARAM;


        if (req.id().empty())
            return ResponseCode::INVALID_PARAM;

        config = make_shared<readlog_config>();
        config->readLogRequest_ = move(req);

        return ResponseCode::OK;
    }

    ReadLogTaskState transform(const shared_ptr<readlog_config> config)
    {
        ReadLogTaskState state;
        assert(!!config);
        state.set_id(config->readLogRequest_.id());
        if (config->log_state_ == decltype(config->log_state_)::stop_by_error)
        {
            state.set_state(TaskState::TASK_STOP_BY_EXCEPTION);
            return state;
        }

        if (config->pause_)
        {
            state.set_state(TaskState::TASK_PAUSED);
            return state;
        }
        else
        {
            state.set_state(TaskState::TASK_RUNNING);
            return state;
        }
    }

    ReadLogManager::ReadLogManager() :readlog_config_manager_(new readlog_config_manager)
    {
    }

    ReadLogManager::~ReadLogManager()
    {
        delete readlog_config_manager_;
    }

    grpc::Status ReadLogManager::createReadLogTaskInvoke(UnaryRpcCreateReadLogTask::RpcReqType&& req, ServerTask::ServerTaskID task_id,
        UnaryRpcCreateReadLogTask::PushTaskResultStreamFuncT push)
    {
#ifdef SERVER_DEBUG
        string debug_temp;
        if (TextFormat::PrintToString(req, &debug_temp))
        {
            LOG_INFO("recv task_id:{}, createReadLogTaskInvoke:{}", task_id, debug_temp);
        }
#endif // SERVER_DEBUG

        UnaryRpcCreateReadLogTask::TaskResultInfo info(new UnaryRpcCreateReadLogTask::TaskResultInfo::element_type);
        info->mutable_header()->set_protocolversion(static_cast<int32_t>(req.header().protocolversion()));

        shared_ptr<readlog_config> config;
        const auto ResponseCode = parse(move(req), config);
        if (ResponseCode != decltype(ResponseCode)::OK)
        {
            info->set_code(ResponseCode);
            info->set_msg("create fail");
            UnaryRpcCreateReadLogTask::TaskResultT re(new UnaryRpcCreateReadLogTask::TaskResultT::element_type(task_id, move(info)));
            push(move(re));
            return grpc::Status::OK;
        }

        const auto result = readlog_config_manager_->start(config);
        if (result == decltype(result)::OK)
        {
            info->set_code(ResponseCode::OK);
            UnaryRpcCreateReadLogTask::TaskResultT re(new UnaryRpcCreateReadLogTask::TaskResultT::element_type(task_id, move(info)));
            push(move(re));
            return grpc::Status::OK;
        }
        else if (result == decltype(result)::ALREADY_CREATE)
        {
            info->set_code(ResponseCode::ALREADY_CREATE);
            info->set_msg("already create, find same request id in server");
            UnaryRpcCreateReadLogTask::TaskResultT re(new UnaryRpcCreateReadLogTask::TaskResultT::element_type(task_id, move(info)));
            push(move(re));
            return grpc::Status::OK;
        }
        else if (result == decltype(result)::INIT_ERR)
        {
            info->set_code(ResponseCode::STOP_BY_EXCEPTION);
            info->set_msg("cant create sub process, check system resource");
            UnaryRpcCreateReadLogTask::TaskResultT re(new UnaryRpcCreateReadLogTask::TaskResultT::element_type(task_id, move(info)));
            push(move(re));
            return grpc::Status::OK;
        }
        else
        {
            return grpc::Status(grpc::StatusCode::UNKNOWN, "create result is't handle by server, logic error");
        }
    }

    grpc::Status ReadLogManager::deleteReadLogTaskInvoke(UnaryRpcDeleteReadLogTask::RpcReqType&& req, ServerTask::ServerTaskID task_id,
        UnaryRpcDeleteReadLogTask::PushTaskResultStreamFuncT push)
    {
#ifdef SERVER_DEBUG
        string debug_temp;
        if (TextFormat::PrintToString(req, &debug_temp))
        {
            LOG_INFO("recv task_id:{}, deleteReadLogTaskInvoke:{}", task_id, debug_temp);
        }
#endif // SERVER_DEBUG

        UnaryRpcDeleteReadLogTask::TaskResultInfo info(new UnaryRpcDeleteReadLogTask::TaskResultInfo::element_type);
        info->mutable_header()->set_protocolversion(static_cast<int32_t>(req.header().protocolversion()));

        const auto result = readlog_config_manager_->stop(req.id());
        if (result == decltype(result)::OK)
        {
            info->set_code(ResponseCode::OK);
            UnaryRpcDeleteReadLogTask::TaskResultT re(new UnaryRpcDeleteReadLogTask::TaskResultT::element_type(task_id, move(info)));
            push(move(re));
            return grpc::Status::OK;
        }
        else if (result == decltype(result)::NOT_EXIST)
        {
            info->set_code(ResponseCode::NOT_EXIST);
            info->set_msg("not exist, check id");
            UnaryRpcDeleteReadLogTask::TaskResultT re(new UnaryRpcDeleteReadLogTask::TaskResultT::element_type(task_id, move(info)));
            push(move(re));
            return grpc::Status::OK;
        }
        else
        {
            return grpc::Status(grpc::StatusCode::UNKNOWN, "delete result is't handle by server, logic error");
        }
    }

    grpc::Status ReadLogManager::pauseReadLogTaskInvoke(UnaryRpcPauseReadLogTask::RpcReqType&& req, ServerTask::ServerTaskID task_id,
        UnaryRpcPauseReadLogTask::PushTaskResultStreamFuncT push)
    {
#ifdef SERVER_DEBUG
        string debug_temp;
        if (TextFormat::PrintToString(req, &debug_temp))
        {
            LOG_INFO("recv task_id:{}, pauseReadLogTaskInvoke:{}", task_id, debug_temp);
        }
#endif // SERVER_DEBUG

        UnaryRpcPauseReadLogTask::TaskResultInfo info(new UnaryRpcPauseReadLogTask::TaskResultInfo::element_type);
        info->mutable_header()->set_protocolversion(static_cast<int32_t>(req.header().protocolversion()));

        const auto result = readlog_config_manager_->pause(req.id());
        if (result == decltype(result)::OK)
        {
            info->set_code(ResponseCode::OK);
            UnaryRpcPauseReadLogTask::TaskResultT re(new UnaryRpcPauseReadLogTask::TaskResultT::element_type(task_id, move(info)));
            push(move(re));
            return grpc::Status::OK;
        }
        else if (result == decltype(result)::NOT_EXIST)
        {
            info->set_code(ResponseCode::NOT_EXIST);
            info->set_msg("not exist, check id");
            UnaryRpcPauseReadLogTask::TaskResultT re(new UnaryRpcPauseReadLogTask::TaskResultT::element_type(task_id, move(info)));
            push(move(re));
            return grpc::Status::OK;
        }
        else if (result == decltype(result)::PAUSED)
        {
            info->set_code(ResponseCode::PAUSED);
            info->set_msg("already paused");
            UnaryRpcPauseReadLogTask::TaskResultT re(new UnaryRpcPauseReadLogTask::TaskResultT::element_type(task_id, move(info)));
            push(move(re));
            return grpc::Status::OK;
        }
        else
        {
            return grpc::Status(grpc::StatusCode::UNKNOWN, "pause result is't handle by server, logic error");
        }
    }

    grpc::Status ReadLogManager::resumeReadLogTaskInvoke(UnaryRpcResumeReadLogTask::RpcReqType&& req, ServerTask::ServerTaskID task_id,
        UnaryRpcResumeReadLogTask::PushTaskResultStreamFuncT push)
    {
#ifdef SERVER_DEBUG
        string debug_temp;
        if (TextFormat::PrintToString(req, &debug_temp))
        {
            LOG_INFO("recv task_id:{}, resumeReadLogTaskInvoke:{}", task_id, debug_temp);
        }
#endif // SERVER_DEBUG

        UnaryRpcResumeReadLogTask::TaskResultInfo info(new UnaryRpcResumeReadLogTask::TaskResultInfo::element_type);
        info->mutable_header()->set_protocolversion(static_cast<int32_t>(req.header().protocolversion()));

        const auto result = readlog_config_manager_->resume(req.id());
        if (result == decltype(result)::OK)
        {
            info->set_code(ResponseCode::OK);
            UnaryRpcResumeReadLogTask::TaskResultT re(new UnaryRpcResumeReadLogTask::TaskResultT::element_type(task_id, move(info)));
            push(move(re));
            return grpc::Status::OK;
        }
        else if (result == decltype(result)::NOT_EXIST)
        {
            info->set_code(ResponseCode::NOT_EXIST);
            info->set_msg("not exist, check id");
            UnaryRpcResumeReadLogTask::TaskResultT re(new UnaryRpcResumeReadLogTask::TaskResultT::element_type(task_id, move(info)));
            push(move(re));
            return grpc::Status::OK;
        }
        else if (result == decltype(result)::RUNNING)
        {
            info->set_code(ResponseCode::RUNNING);
            info->set_msg("is running now");
            UnaryRpcResumeReadLogTask::TaskResultT re(new UnaryRpcResumeReadLogTask::TaskResultT::element_type(task_id, move(info)));
            push(move(re));
            return grpc::Status::OK;
        }
        else
        {
            return grpc::Status(grpc::StatusCode::UNKNOWN, "resume result is't handle by server, logic error");
        }
    }

    grpc::Status ReadLogManager::listReadLogTaskStatesInvoke(UnaryRpcListReadLogTaskStates::RpcReqType&& req, ServerTask::ServerTaskID task_id,
        UnaryRpcListReadLogTaskStates::PushTaskResultStreamFuncT push)
    {
#ifdef SERVER_DEBUG
        string debug_temp;
        if (TextFormat::PrintToString(req, &debug_temp))
        {
            LOG_INFO("recv task_id:{}, listReadLogTaskStatesInvoke:{}", task_id, debug_temp);
        }
#endif // SERVER_DEBUG

        UnaryRpcListReadLogTaskStates::TaskResultInfo info(new UnaryRpcListReadLogTaskStates::TaskResultInfo::element_type);
        info->mutable_header()->set_protocolversion(static_cast<int32_t>(req.header().protocolversion()));

        const auto configs = readlog_config_manager_->list_all();

        info->set_code(ResponseCode::OK);
        for (const auto& config : configs)
            info->mutable_taskstate()->Add(transform(config));

        UnaryRpcListReadLogTaskStates::TaskResultT re(new UnaryRpcListReadLogTaskStates::TaskResultT::element_type(task_id, move(info)));
        push(move(re));
        return grpc::Status::OK;
    }

    grpc::Status ReadLogManager::getReadLogTaskStateInvoke(UnaryRpcGetReadLogTaskState::RpcReqType&& req, ServerTask::ServerTaskID task_id,
        UnaryRpcGetReadLogTaskState::PushTaskResultStreamFuncT push)
    {
#ifdef SERVER_DEBUG
        string debug_temp;
        if (TextFormat::PrintToString(req, &debug_temp))
        {
            LOG_INFO("recv task_id:{}, getReadLogTaskStateInvoke:{}", task_id, debug_temp);
        }
#endif // SERVER_DEBUG

        UnaryRpcGetReadLogTaskState::TaskResultInfo info(new UnaryRpcGetReadLogTaskState::TaskResultInfo::element_type);
        info->mutable_header()->set_protocolversion(static_cast<int32_t>(req.header().protocolversion()));

        const auto config = readlog_config_manager_->get(req.id());
        if (config)
        {
            info->set_code(ResponseCode::OK);
            info->mutable_taskstate()->Add(transform(config));
            UnaryRpcGetReadLogTaskState::TaskResultT re(new UnaryRpcGetReadLogTaskState::TaskResultT::element_type(task_id, move(info)));
            push(move(re));
            return grpc::Status::OK;
        }
        else
        {
            info->set_code(ResponseCode::NOT_EXIST);
            UnaryRpcGetReadLogTaskState::TaskResultT re(new UnaryRpcGetReadLogTaskState::TaskResultT::element_type(task_id, move(info)));
            push(move(re));
            return grpc::Status::OK;
        }
        }

    grpc::Status ReadLogManager::pullReadLogInvoke(const StreamToClient::RpcReqType& req, ServerTask::ServerTaskID task_id,
        StreamToClient::TaskResultT& log_result)
    {
        StreamToClient::TaskResultInfo info(new StreamToClient::TaskResultInfo::element_type);
        //info->mutable_header()->set_protocolversion(static_cast<int32_t>(req.header().protocolversion()));

        const auto result = readlog_config_manager_->pullReadLog(static_cast<int32_t>(req.header().protocolversion()), req.id(), info);
        if (result == decltype(result)::OK)
        {
            StreamToClient::TaskResultT re(new StreamToClient::TaskResultT::element_type(task_id, move(info)));
            log_result = move(re);
            return grpc::Status::OK;
        }
        else if (result == decltype(result)::NOT_EXIST)
        {
            info->mutable_header()->set_protocolversion(static_cast<int32_t>(req.header().protocolversion()));
            info->set_code(ResponseCode::NOT_EXIST);
            StreamToClient::TaskResultT re(new StreamToClient::TaskResultT::element_type(task_id, move(info)));
            log_result = move(re);
            return grpc::Status::OK;
        }
        else
        {
            return grpc::Status(grpc::StatusCode::UNKNOWN, "pullReadLog result is't handle by server, logic error");
        }
        }

    void ReadLogManager::pullReadLogFinished(const ServerTask::ServerTaskID& task_id)
    {
#ifdef SERVER_DEBUG
        LOG_INFO("pullReadLogFinished task_id:{}", task_id);
#endif // SERVER_DEBUG
    }

    grpc::Status ReadLogManager::pushReadLogInvoke(UnaryPushReadLogTask::RpcReqType&& req, ServerTask::ServerTaskID task_id,
        UnaryPushReadLogTask::PushTaskResultStreamFuncT push)
    {
#ifdef SERVER_DEBUG
        string debug_temp;
        if (TextFormat::PrintToString(req, &debug_temp))
        {
            LOG_INFO("recv task_id:{}, pushReadLogInvoke:{}", task_id, debug_temp);
        }
#endif // SERVER_DEBUG

        UnaryPushReadLogTask::TaskResultInfo info(new UnaryPushReadLogTask::TaskResultInfo::element_type);
        readlog_config_manager_->pushReadLog(move(req), info);
        UnaryPushReadLogTask::TaskResultT re(new UnaryPushReadLogTask::TaskResultT::element_type(task_id, move(info)));
        push(move(re));

        return grpc::Status::OK;
    }

    }


