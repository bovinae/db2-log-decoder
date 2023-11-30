#pragma once

#include <string>
#include <map>

namespace tapdata
{
	struct PendingScnWrap
    {
        PendingScnWrap() {}

        void add(const std::string& scn, const std::string& transaction_id)
        {
            pending_scns_[scn] = transaction_id;
        }

        void remove(const std::string& transaction_id)
        {
            for (auto it = pending_scns_.begin(); it != pending_scns_.end(); )
            {
                if (it->second == transaction_id)
                {
                    it = pending_scns_.erase(it);
                } 
                else
                {
                    it++;
                }
            }
        }

        std::string get_min_scn()
        {
            if (pending_scns_.empty())
            {
                return "";
            }
            return pending_scns_.begin()->first;
        }

    private:
        // pending scn -> transaction id
        std::map<std::string, std::string> pending_scns_;
    };
}
