#pragma once
#include <memory>
#include <mutex>
#include <vector>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "DB2PlugInDataSource.grpc.pb.h"
#include "ServerTask.h"
#include "local_config.h"

namespace tapdata
{
    class GrpcServer
    {
    public:
        GrpcServer(const server_config&);
        ~GrpcServer();

        DB2ReadLogServer::AsyncService* getAsyncService()
        {
            return async_ser_.get();
        }

        grpc::ServerCompletionQueue* getServerCompletionQueue()
        {
            return cq_.get();
        }

        void addServerTask(std::unique_ptr<ServerTask>&& context)
        {
            server_tasks_.push_back(std::move(context));
        }

        void run();

    private:
        void thread_run(decltype(server_config::accept_threads_) accept_thread);

    private:
        const server_config server_config_;
        std::unique_ptr<grpc::Server> server_;
        std::unique_ptr<grpc::ServerCompletionQueue> cq_;
        std::unique_ptr<DB2ReadLogServer::AsyncService> async_ser_;
        std::vector<std::unique_ptr<ServerTask>> server_tasks_;

    private:
        struct PerThreadShutdownState
        {
            mutable std::mutex mutex_;
            bool shutdown_;
            PerThreadShutdownState() : shutdown_(false) {}
        };
        std::vector<std::thread> threads_;
        std::vector<std::unique_ptr<PerThreadShutdownState>> shutdown_state_;
    };
}

