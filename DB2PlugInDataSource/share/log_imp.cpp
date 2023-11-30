#include "spdlog/common.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "log_imp.h"

bool g_is_console = false;

//控制台输出
std::shared_ptr<spdlog::logger> g_console_log = nullptr;

//日志输出
std::shared_ptr<spdlog::logger> g_daily_log = nullptr;