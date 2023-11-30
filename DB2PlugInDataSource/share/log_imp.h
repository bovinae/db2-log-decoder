#pragma once

#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "tool.h"

extern std::shared_ptr<spdlog::logger> g_console_log;
extern std::shared_ptr<spdlog::logger> g_daily_log;
extern bool g_is_console;
#ifndef SPDLOG_STR_H
#define SPDLOG_STR_H(x) #x
#endif

#ifndef SPDLOG_STR_HELPER
#define SPDLOG_STR_HELPER(x) SPDLOG_STR_H(x)
#endif

#define LOG_INIT(name,folder)                                            \
    {                                                             \
        g_daily_log = spdlog::daily_logger_mt(name, fmt::format("{}{}/{}.log",tool::get_exe_path(), folder, "log"), 0, 0, false, 30); \
        g_console_log = spdlog::stdout_color_mt("C");             \
    }

#define LOG_SET_LEVEL_AND_OUTPUT(log_level, is_print) \
    {                                                 \
        spdlog::set_level(log_level);                 \
        spdlog::flush_on(log_level);                  \
        g_is_console = is_print;                      \
    }

#ifdef LOG_TRACE
#undef LOG_TRACE
#endif 
#define LOG_TRACE(...)                                                                                    \
    {                                                                                                     \
        if (g_is_console && g_console_log)                                                                \
            g_console_log->trace(__FILE__ ":" SPDLOG_STR_HELPER(__LINE__) " :: " __VA_ARGS__); \
        if (g_daily_log)                                                                                    \
            g_daily_log->trace(__FILE__ ":" SPDLOG_STR_HELPER(__LINE__) " :: " __VA_ARGS__);     \
    }

#ifdef LOG_DEBUG
#undef LOG_DEBUG
#endif 
#define LOG_DEBUG(...)                                                                                    \
    {                                                                                                     \
        if (g_is_console && g_console_log)                                                                \
            g_console_log->debug(__FILE__ ":" SPDLOG_STR_HELPER(__LINE__) " :: " __VA_ARGS__); \
        if (g_daily_log)                                                                                    \
            g_daily_log->debug(__FILE__ ":" SPDLOG_STR_HELPER(__LINE__) " :: " __VA_ARGS__);     \
    }


#ifdef LOG_INFO
#undef LOG_INFO
#endif 
#define LOG_INFO(...)                                                                                    \
    {                                                                                                    \
        if (g_is_console && g_console_log)                                                               \
            g_console_log->info(__FILE__ ":" SPDLOG_STR_HELPER(__LINE__) " :: " __VA_ARGS__); \
        if (g_daily_log)                                                                                   \
            g_daily_log->info(__FILE__ ":" SPDLOG_STR_HELPER(__LINE__) " :: " __VA_ARGS__);     \
    }

#ifdef LOG_WARN
#undef LOG_WARN
#endif 
#define LOG_WARN(...)                                                                                    \
    {                                                                                                    \
        if (g_is_console && g_console_log)                                                               \
            g_console_log->warn(__FILE__ ":" SPDLOG_STR_HELPER(__LINE__) " :: " __VA_ARGS__); \
        if (g_daily_log)                                                                                   \
            g_daily_log->warn(__FILE__ ":" SPDLOG_STR_HELPER(__LINE__) " :: " __VA_ARGS__);     \
    }


#ifdef LOG_ERROR
#undef LOG_ERROR
#endif 
#define LOG_ERROR(...)                                                                                    \
    {                                                                                                     \
        if (g_is_console && g_console_log)                                                                \
            g_console_log->error(__FILE__ ":" SPDLOG_STR_HELPER(__LINE__) " :: " __VA_ARGS__); \
        if (g_daily_log)                                                                                    \
            g_daily_log->error(__FILE__ ":" SPDLOG_STR_HELPER(__LINE__) " :: " __VA_ARGS__);     \
    }

