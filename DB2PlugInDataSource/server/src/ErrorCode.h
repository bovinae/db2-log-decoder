#pragma once
#include <cstdint>
namespace tapdata
{
    enum class ErrorCode :uint32_t
    {
        OK,
        PARAM_ERR,//参数错误
        INIT_ERR,//初始化错误
        FILE_ERR,//文件操作错误
        ALREADY_CREATE,//已经建立
        NOT_EXIST,//不存在
        PAUSED,//已经是暂停状态
        RUNNING//已经是运行状态
    };
}