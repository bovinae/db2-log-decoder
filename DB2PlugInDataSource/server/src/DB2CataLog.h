#pragma once
#include <string>
#include <utility>
namespace tapdata
{
    namespace DB2CataLog
    {
        void clear_all();

        //有可能为空,表示异常
        std::pair<std::string, std::string> create_connect(const std::string& hostname, const std::string& service_name, const std::string& db_name);

        void delete_connect(const std::pair<std::string, std::string>& node_alias);
    }
}