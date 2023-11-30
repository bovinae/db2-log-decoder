#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace tapdata
{
    struct app_log_config
    {
        bool console_{ false };
        int level_{ 2 };
        int jump_size_{ 1000 };
    };

    struct server_config
    {
        std::uint32_t loop_wait_ms_{ 200 };
        std::uint16_t accept_threads_{ 16 };
        std::uint16_t qps_{ 4 };
        std::string server_address_;
    };

    class local_config
    {
    public:
        local_config(const std::string& json_file_path) noexcept;
        const app_log_config& get_app_log_config() const noexcept { return app_log_config_; }
        const server_config& get_server_config() const noexcept { return server_config_; }

    private:
        app_log_config app_log_config_;
        server_config server_config_;
    };
}
