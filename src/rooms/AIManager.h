#ifndef AIMANAGER_H
#define AIMANAGER_H

#include <list>
#include <mutex>
#include <atomic>
#include <thread>
#include "AI.h"

class Room;

class AIManager {
	std::list<AI> bots;
	std::mutex listMutex;
	std::thread aiThread;
	std::atomic<bool> terminateThread{false};

public:
	void add(Room&);
	void clear();
	void threadRun();
	std::size_t count() { return bots.size(); }

	std::vector<std::pair<uint16_t, std::string>> getBots();
};

#endif
