#pragma once
#include <cstdint>
#include <functional>
#include "noncopyable.h"

namespace tapdata
{
    class ServerTask :public noncopyable
    {
    public:
        using ChangeServerTaskFuncT = std::function<bool(bool)>;
        using ServerTaskID = uint64_t;
        ServerTask() :noncopyable()
        {
        }

        virtual ~ServerTask()
        {
        }

        virtual void prepare_stop() = 0;

    protected:
        enum class ServerTaskState { RequestDone, ReadDone, WriteDone, FinishDone };
    };
}

