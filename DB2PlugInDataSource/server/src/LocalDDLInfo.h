#pragma once
#include <string>
namespace tapdata
{
	struct LocalDDLInfo
	{
		std::string schema;
		std::string tableName;
		std::string option;//总是大写
		std::string option_next_word;//总是大写

		operator std::string() const
		{
			return "schema:" + schema + ", table:" + tableName + ", option:" + option + ", option_next_word:" + option_next_word;
		}

		bool setInfo(const std::string& default_schema, const std::string& ddl);
	};
}