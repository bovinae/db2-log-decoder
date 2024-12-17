#include "json.hpp"
#include "local_config.h"
#include "json_define.h"
#include "log_imp.h"
#include "tool.h"

#ifdef V9_CONFIG_SERVER_ADDRESS
#define LOCAL_CONFIG_SERVER_ADDRESS "0.0.0.0:50050"
#elif V10_CONFIG_SERVER_ADDRESS
#define LOCAL_CONFIG_SERVER_ADDRESS "0.0.0.0:50051"
#else
#define LOCAL_CONFIG_SERVER_ADDRESS "0.0.0.0:50051"
#endif

namespace tapdata
{
#define NOW_VERSION 0
    int config_version_array[] = { 0 };
    using namespace std;
    using json = nlohmann::json;

    struct app_log_config_imp
    {
        app_log_config_imp(app_log_config* pconfig) :app_log_config_{ pconfig } {};

        void init_def_value() noexcept
        {
            app_log_config_->console_ = true;
            app_log_config_->level_ = 1;
            app_log_config_->jump_size_ = 1000;
            app_log_config_->log_db2_data_ = false;
        }

        bool init_value_from_json(int version, const nlohmann::json& j) noexcept
        {
            if (version == config_version_array[NOW_VERSION])
            {
                try
                {
                    app_log_config_->console_ = j.at(json_console);
                    app_log_config_->level_ = j.at(json_level);
                    app_log_config_->jump_size_ = j.at(json_jump_size);
                    app_log_config_->log_db2_data_ = j.at(json_log_db2_data);
                }
                catch (const json::exception& ex)
                {
                    LOG_WARN("init value from json fail, what:{}", ex.what());
                    return false;
                }
                return true;
            }
            else
                return false;
        }

        nlohmann::json build_json() noexcept
        {
            json j;
            j[json_console] = app_log_config_->console_;
            j[json_level] = app_log_config_->level_;
            j[json_jump_size] = app_log_config_->jump_size_;
            j[json_log_db2_data] = app_log_config_->log_db2_data_;
            return j;
        }

        bool is_valid() noexcept
        {
            return app_log_config_->level_ >= 0 && app_log_config_->level_ < 7 && app_log_config_->jump_size_>=10 && app_log_config_->jump_size_<=5000;
        }

        bool fix() noexcept
        {
            return false;
        }

    private:
        app_log_config* app_log_config_{};
    };

    struct server_config_imp
    {
        server_config_imp(server_config* pconfig) :server_config_{ pconfig } {};

        void init_def_value() noexcept
        {
            server_config c;
            swap(*server_config_, c);
            server_config_->server_address_ = LOCAL_CONFIG_SERVER_ADDRESS;
        }

        bool init_value_from_json(int version, const nlohmann::json& j) noexcept
        {
            if (version == config_version_array[NOW_VERSION])
            {
                try
                {
                    server_config_->accept_threads_ = j.at(json_accept_threads);
                    server_config_->loop_wait_ms_ = j.at(json_loop_wait_ms);
                    server_config_->qps_ = j.at(json_qps);
                    server_config_->server_address_ = j.at(json_server_address);
                }
                catch (const json::exception& ex)
                {
                    LOG_WARN("init value from json fail, what:{}", ex.what());
                    return false;
                }
                return true;
            }
            else
                return false;
        }

        nlohmann::json build_json() noexcept
        {
            json j;
            j[json_accept_threads] = server_config_->accept_threads_;
            j[json_loop_wait_ms] = server_config_->loop_wait_ms_;
            j[json_qps] = server_config_->qps_;
            j[json_server_address] = server_config_->server_address_;
            return j;
        }

        bool is_valid() noexcept
        {
            return server_config_->accept_threads_ > 0 && server_config_->accept_threads_ <= 40 &&
                server_config_->loop_wait_ms_ > 0 && server_config_->loop_wait_ms_ <= 5000 &&
                server_config_->qps_ > 0 && server_config_->qps_ <= 60 &&
                !server_config_->server_address_.empty();
        }

        bool fix() noexcept
        {
            if (server_config_->server_address_.empty())
                return false;

            if (!server_config_->accept_threads_)
                server_config_->accept_threads_ = 1;

            if (server_config_->accept_threads_ > 40)
                server_config_->accept_threads_ = 40;

            if (!server_config_->loop_wait_ms_)
                server_config_->loop_wait_ms_ = 1;

            if (server_config_->loop_wait_ms_ > 5000)
                server_config_->loop_wait_ms_ = 5000;

            if (!server_config_->qps_)
                server_config_->qps_ = 1;

            if (server_config_->qps_ > 60)
                server_config_->qps_ = 60;

            return true;
        }

    private:
        server_config* server_config_{};
    };

    template<class T>
    bool load_info(T* pthis, int version, const nlohmann::json& j, const char* node) noexcept
    {
        bool result = true;

        if (!j.contains(node) || !pthis->init_value_from_json(version, j[node]))
        {
            //从json装载配置失败
            LOG_WARN("{} init value from json fail", node);
            pthis->init_def_value();//使用默认值
            result = false;
        }
        else
        {
            //从json装载配置成功
            if (!pthis->is_valid())
            {
                //json配置校验不成功
                LOG_WARN("{} info invalid", node);
                if (!pthis->fix())
                {
                    //尝试修复失败
                    LOG_WARN("{} fix fail, use default value", node);
                    pthis->init_def_value();//使用默认值
                }
                else
                {
                    LOG_WARN("{} fix succ", node);
                }
                result = false;
            }
        }

        return result;
    }

    json load_json_from_config_file(const string& json_file_path_str) noexcept //从文件读入json
    {
        json j;
        vector<char> json_data;
        if (!tool::read_whole_file(json_file_path_str.c_str(), json_data))
        {
            LOG_ERROR("read file error, file_path:{}", json_file_path_str.c_str());
            return j;
        }

        try
        {
            j = json::parse(json_data);
        }
        catch (const json::exception& ex)
        {
            LOG_ERROR("parse json data fail, what:{}", ex.what());
        }

        return j;
    }

    void save_json_to_config_file(const json& j, const string& json_file_path_str)//json写入文件
    {
        auto str = j.dump(4);

        if (!tool::write_whole_file(json_file_path_str.c_str(), str.data(), str.size()))
        {
            LOG_WARN("write file fail, path:{}", json_file_path_str.c_str());
        }
    }

    //找到从配置文件内读出json最后一个已知版本，如果没找到返回-1
    int get_last_version_str(const json& j) noexcept
    {
        if (j.empty())
            return -1;
        if (!j.contains(json_version) || !j[json_version].is_number_integer())
            return -1;
        int version = j[json_version];
        if (version < 0)
            return -1;

        auto begin = std::begin(config_version_array);
        auto end = std::end(config_version_array);
        std::reverse_iterator<decltype(std::begin(config_version_array))> bb(end);
        std::reverse_iterator<decltype(std::begin(config_version_array))> ee(begin);
        auto it = find(bb, ee, version);
        if (it != ee)
            return *it;

        return -1;
    }

    local_config::local_config(const string& json_file_path_str) noexcept
    {
        json j = load_json_from_config_file(json_file_path_str);
        app_log_config_imp app_log_config_imp{ &app_log_config_ };
        server_config_imp server_config_imp{ &server_config_ };

        bool result{ true };
        if (j.empty())
        {
            result &= load_info(&app_log_config_imp, config_version_array[NOW_VERSION], j, json_app_log_config);
            result &= load_info(&server_config_imp, config_version_array[NOW_VERSION], j, json_server_config);
        }
        else
        {
            result &= load_info(&app_log_config_imp, get_last_version_str(j), j, json_app_log_config);
            result &= load_info(&server_config_imp, get_last_version_str(j), j, json_server_config);
        }

        if (!result)
        {
            json wj;
            wj[json_version] = config_version_array[NOW_VERSION];
            wj[json_app_log_config] = app_log_config_imp.build_json();
            wj[json_server_config] = server_config_imp.build_json();
            save_json_to_config_file(wj, json_file_path_str.c_str());
        }
    }

}


