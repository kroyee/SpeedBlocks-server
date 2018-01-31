#include "AsyncTask.h"
#include <chrono>

namespace AsyncTask {

	namespace detail {
		std::deque<std::future<int>> futureQueue;
	}

	bool empty() { return detail::futureQueue.empty(); }

	void check() {
		while (detail::futureQueue.front().valid()) {
			if (detail::futureQueue.front().wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
				detail::futureQueue.front().get();
				detail::futureQueue.pop_front();
			}
			else break;
		}
	}

	void exit() {
		while (!detail::futureQueue.empty()) {
			if (detail::futureQueue.front().valid()) {
				detail::futureQueue.front().get();
				detail::futureQueue.pop_front();
			}
		}
	}

}