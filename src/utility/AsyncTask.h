#ifndef ASYNCTASK_H
#define ASYNCTASK_H

#include <deque>
#include <future>

namespace AsyncTask {

	namespace detail {
		extern std::deque<std::future<void>> futureQueue;
	}

	template <typename F>
	void add(F func) {
		detail::futureQueue.emplace_back( std::async(std::launch::async, func) );
	}

	template <typename F>
	void addAndRepeat(F func, int repeat = 3) {
		detail::futureQueue.emplace_back( std::async(std::launch::async, [func, repeat]() mutable {
			while (repeat && func())
				--repeat;
		}) );
	}

	template <typename F1, typename F2>
	void addAndReact(F1 func1, F2 func2) {
		detail::futureQueue.emplace_back( std::async(std::launch::async, [func1, func2](){
			func2(func1());
		}) );
	}

	bool empty();
	void check();
	void exit();
}

#endif
