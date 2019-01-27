#include "AsyncTask.h"
#include <chrono>
#include <iostream>

namespace AsyncTask {

	namespace detail {
		std::deque<std::future<void>> futureQueue;
		std::mutex mutex;
	}

	bool empty() {
		std::lock_guard<std::mutex> guard(detail::mutex);
		return detail::futureQueue.empty();
	}

	void check() {
		std::lock_guard<std::mutex> guard(detail::mutex);
		while (!detail::futureQueue.empty() && detail::futureQueue.front().valid()) {
			if (detail::futureQueue.front().wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				detail::futureQueue.front().get();
				detail::futureQueue.pop_front();
			}
			else break;
		}
	}

	void exit() {
		std::lock_guard<std::mutex> guard(detail::mutex);
		while (!detail::futureQueue.empty()) {
			if (detail::futureQueue.front().valid()) {
				detail::futureQueue.front().get();
				detail::futureQueue.pop_front();
			}
		}
	}

}
