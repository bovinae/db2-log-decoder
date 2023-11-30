#include <thread>
#include <algorithm>
#include "DB2PlugInDataApp.h"
#include "version.h"
#include "db2_log_version.h"
#include "DB2CataLog.h"


namespace tapdata
{
    using namespace std;
    DB2PlugInDataApp::DB2PlugInDataApp() :local_app(),
        read_log_manager_(new ReadLogManager),
        grpc_server_(new GrpcServer(local_config_->get_server_config()))
    {
        initGrpcTasks();
    }

    DB2PlugInDataApp::~DB2PlugInDataApp()
    {
        keep_run_ = false;
        if (thread_.joinable())
            thread_.join();
    }

    void DB2PlugInDataApp::initGrpcTasks()
    {
        const auto unary_qps = local_config_->get_server_config().qps_ > 2 ? 2 : local_config_->get_server_config().qps_;//一元rpc最多每个线程2个
        for (decltype(local_config_->get_server_config().accept_threads_) a = 0; a < local_config_->get_server_config().accept_threads_; ++a)
        {
            for (decltype(local_config_->get_server_config().qps_) i = 0; i < unary_qps; ++i)
            {
                grpc_server_->addServerTask(move(unique_ptr<ServerTask>(
                    new UnaryRpcPing(grpc_server_->getAsyncService(), grpc_server_->getServerCompletionQueue(),
                        &UnaryRpcPing::RpcServerType::RequestPing,
                        bind(&DB2PlugInDataApp::pingInvoke, this, placeholders::_1, placeholders::_2, placeholders::_3),
                        local_config_->get_server_config().loop_wait_ms_))));

                grpc_server_->addServerTask(move(unique_ptr<ServerTask>(
                    new UnaryRpcServerInfo(grpc_server_->getAsyncService(), grpc_server_->getServerCompletionQueue(),
                        &UnaryRpcServerInfo::RpcServerType::RequestServerInfo,
                        bind(&DB2PlugInDataApp::serverInfoInvoke, this, placeholders::_1, placeholders::_2, placeholders::_3),
                        local_config_->get_server_config().loop_wait_ms_))));


                grpc_server_->addServerTask(move(unique_ptr<ServerTask>(
                    new UnaryRpcCreateReadLogTask(grpc_server_->getAsyncService(), grpc_server_->getServerCompletionQueue(),
                        &UnaryRpcCreateReadLogTask::RpcServerType::RequestCreateReadLogTask,
                        bind(&DB2PlugInDataApp::createReadLogTaskInvoke, this, placeholders::_1, placeholders::_2, placeholders::_3),
                        local_config_->get_server_config().loop_wait_ms_))));

                grpc_server_->addServerTask(move(unique_ptr<ServerTask>(
                    new UnaryRpcDeleteReadLogTask(grpc_server_->getAsyncService(), grpc_server_->getServerCompletionQueue(),
                        &UnaryRpcDeleteReadLogTask::RpcServerType::RequestDeleteReadLogTask,
                        bind(&DB2PlugInDataApp::deleteReadLogTaskInvoke, this, placeholders::_1, placeholders::_2, placeholders::_3),
                        local_config_->get_server_config().loop_wait_ms_))));

                grpc_server_->addServerTask(move(unique_ptr<ServerTask>(
                    new UnaryRpcPauseReadLogTask(grpc_server_->getAsyncService(), grpc_server_->getServerCompletionQueue(),
                        &UnaryRpcPauseReadLogTask::RpcServerType::RequestPauseReadLogTask,
                        bind(&DB2PlugInDataApp::pauseReadLogTaskInvoke, this, placeholders::_1, placeholders::_2, placeholders::_3),
                        local_config_->get_server_config().loop_wait_ms_))));

                grpc_server_->addServerTask(move(unique_ptr<ServerTask>(
                    new UnaryRpcResumeReadLogTask(grpc_server_->getAsyncService(), grpc_server_->getServerCompletionQueue(),
                        &UnaryRpcResumeReadLogTask::RpcServerType::RequestResumeReadLogTask,
                        bind(&DB2PlugInDataApp::resumeReadLogTaskInvoke, this, placeholders::_1, placeholders::_2, placeholders::_3),
                        local_config_->get_server_config().loop_wait_ms_))));

                grpc_server_->addServerTask(move(unique_ptr<ServerTask>(
                    new UnaryRpcListReadLogTaskStates(grpc_server_->getAsyncService(), grpc_server_->getServerCompletionQueue(),
                        &UnaryRpcListReadLogTaskStates::RpcServerType::RequestListReadLogTaskStates,
                        bind(&DB2PlugInDataApp::listReadLogTaskStatesInvoke, this, placeholders::_1, placeholders::_2, placeholders::_3),
                        local_config_->get_server_config().loop_wait_ms_))));

                grpc_server_->addServerTask(move(unique_ptr<ServerTask>(
                    new UnaryRpcGetReadLogTaskState(grpc_server_->getAsyncService(), grpc_server_->getServerCompletionQueue(),
                        &UnaryRpcGetReadLogTaskState::RpcServerType::RequestGetReadLogTaskState,
                        bind(&DB2PlugInDataApp::getReadLogTaskStateInvoke, this, placeholders::_1, placeholders::_2, placeholders::_3),
                        local_config_->get_server_config().loop_wait_ms_))));

            }

            for (decltype(local_config_->get_server_config().qps_) i = 0; i < local_config_->get_server_config().qps_; ++i)
            {
                grpc_server_->addServerTask(move(unique_ptr<ServerTask>(
                    new StreamToClient(grpc_server_->getAsyncService(), grpc_server_->getServerCompletionQueue(),
                        bind(&DB2PlugInDataApp::pullReadLogInvoke, this, placeholders::_1, placeholders::_2, placeholders::_3),
                        bind(&DB2PlugInDataApp::pullReadLogFinished, this, placeholders::_1),
                        local_config_->get_server_config().loop_wait_ms_))));

                grpc_server_->addServerTask(move(unique_ptr<ServerTask>(
                    new UnaryPushReadLogTask(grpc_server_->getAsyncService(), grpc_server_->getServerCompletionQueue(),
                        &UnaryPushReadLogTask::RpcServerType::RequestPushReadLog,
                        bind(&DB2PlugInDataApp::pushReadLogInvoke, this, placeholders::_1, placeholders::_2, placeholders::_3),
                        local_config_->get_server_config().loop_wait_ms_))));

            }
        }

    }

    grpc::Status DB2PlugInDataApp::pingInvoke(UnaryRpcPing::RpcReqType&& req, ServerTask::ServerTaskID task_id,
        UnaryRpcPing::PushTaskResultStreamFuncT push)
    {
        UnaryRpcPing::TaskResultInfo info(new UnaryRpcPing::TaskResultInfo::element_type);
        info->mutable_header()->set_protocolversion(req.header().protocolversion());
        info->set_msg("ok");
        info->set_status(0);
        UnaryRpcPing::TaskResultT re(new UnaryRpcPing::TaskResultT::element_type(task_id, move(info)));
        push(move(re));
        return grpc::Status::OK;
    }

    grpc::Status DB2PlugInDataApp::serverInfoInvoke(UnaryRpcServerInfo::RpcReqType&&, ServerTask::ServerTaskID task_id,
        UnaryRpcServerInfo::PushTaskResultStreamFuncT push)
    {
        UnaryRpcServerInfo::TaskResultInfo info(new UnaryRpcServerInfo::TaskResultInfo::element_type);
        info->mutable_header()->set_protocolversion(static_cast<int32_t>(LocalProtocolVersion::v1));
        info->set_serverversion(static_cast<int32_t>(LocalServerVersion::v1));
        info->set_info(LocalServerInfo);
        info->add_supportprotocolversion(static_cast<int32_t>(LocalProtocolVersion::v1));
        info->add_supportwritertype(WriterType::GRPC);
        info->set_supportversion(static_cast<DB2Veresion>(LocalDB2LogVersion));
        UnaryRpcServerInfo::TaskResultT re(new UnaryRpcServerInfo::TaskResultT::element_type(task_id, move(info)));
        push(move(re));
        return grpc::Status::OK;
    }

    grpc::Status DB2PlugInDataApp::createReadLogTaskInvoke(UnaryRpcCreateReadLogTask::RpcReqType&& req, ServerTask::ServerTaskID task_id,
        UnaryRpcCreateReadLogTask::PushTaskResultStreamFuncT push)
    {
        return read_log_manager_->createReadLogTaskInvoke(move(req), task_id, push);
    }

    grpc::Status DB2PlugInDataApp::deleteReadLogTaskInvoke(UnaryRpcDeleteReadLogTask::RpcReqType&& req, ServerTask::ServerTaskID task_id,
        UnaryRpcDeleteReadLogTask::PushTaskResultStreamFuncT push)
    {
        return read_log_manager_->deleteReadLogTaskInvoke(move(req), task_id, push);
    }

    grpc::Status DB2PlugInDataApp::pauseReadLogTaskInvoke(UnaryRpcPauseReadLogTask::RpcReqType&& req, ServerTask::ServerTaskID task_id,
        UnaryRpcPauseReadLogTask::PushTaskResultStreamFuncT push)
    {
        return read_log_manager_->pauseReadLogTaskInvoke(move(req), task_id, push);
    }

    grpc::Status DB2PlugInDataApp::resumeReadLogTaskInvoke(UnaryRpcResumeReadLogTask::RpcReqType&& req, ServerTask::ServerTaskID task_id,
        UnaryRpcResumeReadLogTask::PushTaskResultStreamFuncT push)
    {
        return read_log_manager_->resumeReadLogTaskInvoke(move(req), task_id, push);
    }

    grpc::Status DB2PlugInDataApp::listReadLogTaskStatesInvoke(UnaryRpcListReadLogTaskStates::RpcReqType&& req, ServerTask::ServerTaskID task_id,
        UnaryRpcListReadLogTaskStates::PushTaskResultStreamFuncT push)
    {
        return read_log_manager_->listReadLogTaskStatesInvoke(move(req), task_id, push);
    }

    grpc::Status DB2PlugInDataApp::getReadLogTaskStateInvoke(UnaryRpcGetReadLogTaskState::RpcReqType&& req, ServerTask::ServerTaskID task_id,
        UnaryRpcGetReadLogTaskState::PushTaskResultStreamFuncT push)
    {
        return read_log_manager_->getReadLogTaskStateInvoke(move(req), task_id, push);
    }

    grpc::Status DB2PlugInDataApp::pullReadLogInvoke(const StreamToClient::RpcReqType& req, ServerTask::ServerTaskID task_id, StreamToClient::TaskResultT& result)
    {
        return read_log_manager_->pullReadLogInvoke(req, task_id, result);
    }

    grpc::Status DB2PlugInDataApp::pushReadLogInvoke(UnaryPushReadLogTask::RpcReqType&& req, ServerTask::ServerTaskID task_id, UnaryPushReadLogTask::PushTaskResultStreamFuncT push)
    {
        return read_log_manager_->pushReadLogInvoke(move(req), task_id, push);
    }

    void DB2PlugInDataApp::pullReadLogFinished(const ServerTask::ServerTaskID& task_id)
    {
        return read_log_manager_->pullReadLogFinished(task_id);
    }

    void DB2PlugInDataApp::run() noexcept
    {
        thread_ = thread{ &DB2PlugInDataApp::thread_func, this };
    }

    void DB2PlugInDataApp::sig_term_proc(int sig_no)
    {
        local_app::sig_term_proc(sig_no);
    }

    void DB2PlugInDataApp::sig_crash_proc(int sig_no)
    {
        local_app::sig_crash_proc(sig_no);
    }

    void DB2PlugInDataApp::sig_dump_proc(int sig_no)
    {
        local_app::sig_dump_proc(sig_no);
    }

    void DB2PlugInDataApp::thread_func() noexcept
    {
        keep_run_ = true;
        DB2CataLog::clear_all();
        grpc_server_->run();
        while (keep_run_)
        {
            this_thread::sleep_for(500ms);
        }
    }


}

