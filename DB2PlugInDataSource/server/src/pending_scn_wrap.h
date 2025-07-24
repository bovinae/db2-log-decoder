#pragma once

#include <string>
#include <map>
#include <vector>
#include "log_imp.h"

namespace tapdata
{
    struct PendingScnWrap
    {
        PendingScnWrap() {}

        void add(const std::string& scn, const std::string& transaction_id)
        {
            //LOG_DEBUG("[PendingScnWrap] scn:{}, tid:{}", scn, transaction_id);
            auto it = pending_scns_.insert({scn, transaction_id}).first;
	    	pending_tid_it_[transaction_id].push_back(it);
        }

        void remove(const std::string& transaction_id)
        {
		    if (pending_tid_it_.find(transaction_id) != pending_tid_it_.end()) {
		    	auto vec = pending_tid_it_[transaction_id];
				for (auto it = vec.begin(); it != vec.end(); it++) {
				    pending_scns_.erase(*it);
				}
				pending_tid_it_.erase(transaction_id);
		    }
/*
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
*/
        }

        std::string get_min_scn()
        {
            if (pending_scns_.empty())
            {
				//LOG_DEBUG("get empty min scn");
                return "";
            }
            //LOG_DEBUG("get min scn:{}", pending_scns_.begin()->first);
            return pending_scns_.begin()->first;
        }

    private:
        // pending scn -> transaction id
        std::map<std::string, std::string> pending_scns_;
        // transaction id -> iterator
        std::map<std::string, std::vector<std::map<std::string, std::string>::iterator> > pending_tid_it_;
    };
}
