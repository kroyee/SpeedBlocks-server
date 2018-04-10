#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include <deque>
#include <functional>
#include <mutex>

namespace TaskQueue {
	void push(uint8_t queue_nr, std::function<void()> func);
	void perform(uint8_t queue_nr);
}

#endif
