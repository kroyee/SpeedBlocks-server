#ifndef AIMANAGER_H
#define AIMANAGER_H

#include <list>
#include <mutex>
#include <atomic>
#include <thread>
#include "AI.h"

class AIManager {
	std::list<AI> bots;
	int count=0;
	std::mutex listMutex;
	sf::Clock& gameclock;
	std::thread t;
	std::atomic<bool> terminateThread;
	std::atomic<uint8_t> alive;
	Signal<void, uint16_t, RoundStats&, uint16_t>& botSendLines;
public:
	AIManager(sf::Clock& _gameclock, Signal<void, uint16_t, RoundStats&, uint16_t>& _sendLines);
	void setAmount(unsigned int amount);
	void threadRun();
	int getCount() { return count; }
	void sendLines(uint16_t senderid, float amount);

	void startRound();
	void startCountDown();

	AI* getBot(uint16_t id);
};

#endif