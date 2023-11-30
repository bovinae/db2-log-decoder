#include <regex>
#include "LocalDDLInfo.h"
#include "tool.h"

using namespace std;
namespace tapdata
{
	bool parseAlertTableDDL(const string& ddl, string& schema, string& table, string& option, string& option_next_word)
	{
		static const regex reg(
			R"-(\s*ALTER\s+TABLE\s+("[ a-zA-Z0-9@#_$]+"|[A-Z0-9@#_$]+)(?:\s*\.\s*("[ a-zA-Z0-9@#_$]+"|[A-Z0-9@#_$]+))?\s+([A-Za-z]+)\s+((?:"[ a-zA-Z0-9@#_$]+"|[A-Z0-9@#_$]+)).*)-", regex::icase
		);
		try
		{
			smatch m;
			if (!regex_match(ddl, m, reg))
				return false;
			if (m.size() != 5)
				return false;
			const string m2 = m[2];
			if (m2.empty())
			{
				table = m[1];
			}
			else
			{
				schema = m[1];
				table = m[2];
			}
			option = m[3];
			option_next_word = m[4];
			return true;
		}
		catch (const exception& ex)
		{
			return false;
		}
	}
    
	bool parseCommentDDL(const string& ddl, string& schema, string& table, string& option, string& option_next_word)
	{
		using namespace std;
		static const regex reg(
			R"-(\s*COMMENT\s+ON\s+([A-Za-z]+)\s+("[ a-zA-Z0-9@#_$]+"|[A-Z0-9@#_$]+)\s*\.(?:\s*("[ a-zA-Z0-9@#_$]+"|[A-Z0-9@#_$]+)\s*\.)?\s*(?:"[ a-zA-Z0-9@#_$]+"|[A-Z0-9@#_$]+).*)-", regex::icase
		);
		try
		{
			smatch m;
			if (!regex_match(ddl, m, reg))
				return false;

			if (m.size() != 4)
				return false;

			option_next_word = m[1];
			option = "COMMENT";
			const string m3 = m[3];
			if (m3.empty())
			{
				table = m[2];
			}
			else
			{
				schema = m[2];
				table = m[3];
			}

			return true;
		}
		catch (const exception& ex)
		{
			return false;
		}
	}

	bool tapdata::LocalDDLInfo::setInfo(const string& default_schema, const string& ddl)
	{
        string input = ddl;
        tool::replace_chars(input, { '\r','\n' }, ' ');
        if (!parseAlertTableDDL(input, schema, tableName, option, option_next_word) && !parseCommentDDL(input, schema, tableName, option, option_next_word))
            return false;
        if (schema.empty())
            schema = default_schema;

		transform(option.begin(), option.end(), option.begin(), ::toupper);
		transform(option_next_word.begin(), option_next_word.end(), option_next_word.begin(), ::toupper);
        return true;
	}
}
