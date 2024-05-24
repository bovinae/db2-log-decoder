#include <chrono>
#include "GrpcServer.h"
#include "log_imp.h"

namespace tapdata
{
    using namespace std;
    GrpcServer::GrpcServer(const server_config& config) :
        server_config_(config),
        async_ser_(new DB2ReadLogServer::AsyncService)
    {
        grpc::ServerBuilder builder;
        builder.SetMaxSendMessageSize(100 * 1024 * 1024);//100MB
        builder.SetMaxReceiveMessageSize(100 * 1024 * 1024);//100MB
        builder.AddListeningPort(server_config_.server_address_, grpc::InsecureServerCredentials());
        builder.RegisterService(async_ser_.get());
        cq_ = builder.AddCompletionQueue();
        server_ = builder.BuildAndStart();
    }

    GrpcServer::~GrpcServer()
    {
        for (auto& c : server_tasks_)
            c->prepare_stop();

        for (auto& ss : shutdown_state_)
        {
            lock_guard<decltype(ss->mutex_)> lock(ss->mutex_);
            ss->shutdown_ = true;
        }

        server_->Shutdown(chrono::system_clock::now() + chrono::seconds(3));
        cq_->Shutdown();

        for (auto& th : threads_)
            th.join();

        bool ok;
        void* got_tag;
        while (cq_->Next(&got_tag, &ok))
        {
            //do nothing
        }
    }

    void GrpcServer::run()
    {
        for (decltype(server_config_.accept_threads_) accept_thread = 0; accept_thread < server_config_.accept_threads_; accept_thread++)
        {
            shutdown_state_.emplace_back(new PerThreadShutdownState());
            threads_.emplace_back(&GrpcServer::thread_run, this, accept_thread);
        }
    }

    void GrpcServer::thread_run(decltype(server_config::accept_threads_) accept_thread)
    {
        bool ok;
        void* got_tag;
        LOG_DEBUG("enter grpcserver thread_run, accept_thread:{}", accept_thread);
        if (!cq_->Next(&got_tag, &ok)) {
            LOG_DEBUG("the queue is fully drained and shut down, accept_thread:{}", accept_thread);
            return;
        }
        LOG_DEBUG("got an event, accept_thread:{}", accept_thread);

        ServerTask::ChangeServerTaskFuncT* stateFunc;
        auto mu_ptr = &shutdown_state_[accept_thread]->mutex_;
        do {
            stateFunc = static_cast<ServerTask::ChangeServerTaskFuncT*>(got_tag);
            mu_ptr->lock();
            if (shutdown_state_[accept_thread]->shutdown_) {
                mu_ptr->unlock();
                return;
            }
        } while (cq_->DoThenAsyncNext(
            [&, stateFunc, ok, mu_ptr]()
            {
                stateFunc[0](ok);
                mu_ptr->unlock();
            },
            &got_tag, &ok,
                //gpr_time_add(gpr_now(GPR_CLOCK_REALTIME), gpr_time_from_millis(1, GPR_TIMESPAN)))
                gpr_inf_future(GPR_CLOCK_REALTIME))
            );
        LOG_DEBUG("connection is SHUTDOWN");
    }



}


