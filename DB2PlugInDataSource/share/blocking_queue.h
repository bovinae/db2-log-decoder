#pragma once
#include <deque>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <cstdint>
#include "noncopyable.h"

namespace tapdata
{
    template<typename T>
    class blocking_queue : noncopyable
    {
    public:
        using queue_type = std::deque<T>;

        void put(const T& x)
        {
            {
                std::lock_guard<decltype(mutex_)> lock(mutex_);
                queue_.push_back(x);
            }

            not_empty_.notify_one();
        }

        void put(T&& x)
        {
            {
                std::lock_guard<decltype(mutex_)> lock(mutex_);
                queue_.push_back(std::move(x));
            }
            not_empty_.notify_one();
        }

        T take()
        {
            std::unique_lock<decltype(mutex_)> lock(mutex_);
            while (queue_.empty())
            {
                not_empty_.wait(lock);
            }
            assert(!queue_.empty());
            T front(std::move(queue_.front()));
            queue_.pop_front();
            return front;
        }

        //if timeout, return false
        bool try_wait_and_pop(T& x, uint32_t _milli)
        {
            std::unique_lock<decltype(mutex_)> lock(mutex_);
            while (queue_.empty())
            {
                if (std::cv_status::timeout == not_empty_.wait_for(lock, std::chrono::milliseconds(_milli)))
                    return false;
            }

            x = std::move(queue_.front());
            queue_.pop_front();
            return true;
        }

        size_t size() const
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            return queue_.size();
        }

        //void awake_whith_invalid_value()
        //{
        //    T x;
        //    {
        //        std::lock_guard<decltype(mutex_)> lock(mutex_);
        //        queue_.clear();
        //        queue_.emplace_back(std::move(x));
        //    }

        //    not_empty_.notify_one();
        //}

    private:
        mutable std::mutex mutex_;
        std::condition_variable not_empty_;
        queue_type queue_;
    };
}
