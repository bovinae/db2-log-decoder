#pragma once
#include <deque>
#include <condition_variable>
#include <algorithm>
#include <iterator>
#include <mutex>
#include <chrono>
#include <cstdint>
#include "noncopyable.h"

namespace tapdata
{
	template<typename T>
	class batch_queue : noncopyable
	{
	public:
		using queue_type = std::deque<T>;

		template < class iterator>
		void put(iterator begin, iterator end)
		{
			{
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				for (iterator it = begin; it != end; ++it)
					queue_.push_back(std::move(*it));
			}
			not_empty_.notify_one();
		}

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

		void try_wait_and_pop(queue_type& out_queue, uint32_t wait_milli, size_t out_size)
		{
			out_queue.clear();
			std::unique_lock<decltype(mutex_)> lock(mutex_);
			while (queue_.size() < out_size)
			{
				if (std::cv_status::timeout == not_empty_.wait_for(lock, std::chrono::milliseconds(wait_milli)))
				{
					if (queue_.empty() || queue_.size() < out_size)
					{
						std::swap(out_queue, queue_);
						return;
					}
				}
			}

			std::move(queue_.begin(), queue_.begin() + out_size, std::back_inserter(out_queue));
			queue_.erase(queue_.begin(), queue_.begin() + out_size);
		}

		size_t size() const
		{
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			return queue_.size();
		}

	private:
		mutable std::mutex mutex_;
		std::condition_variable not_empty_;
		queue_type queue_;
	};
}
