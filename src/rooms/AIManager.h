#ifndef AIMANAGER_H
#define AIMANAGER_H

#include <list>
#include <mutex>
#include <atomic>
#include <thread>
#include "AI.h"
#include "PacketCompress.h"

class Room;

class AIManager {
	std::list<AI> bots;
	std::mutex listMutex;
	sf::Clock& gameclock;
	std::thread aiThread;
	std::atomic<bool> terminateThread;
	std::atomic<uint8_t> alive;
	Signal<void, uint16_t, RoundStats&, uint16_t>& botSendLines;

	PacketCompress compressor;
public:
	AIManager(sf::Clock& _gameclock, Signal<void, uint16_t, RoundStats&, uint16_t>& _sendLines);
	void setAmount(unsigned int amount);
	void threadRun();
	std::size_t count() { return bots.size(); }
	void sendLines(uint16_t senderid, float amount);

	void startRound();
	void endRound(const sf::Time& t);
	void startCountdown(uint16_t seed1, uint16_t seed2);
	void countDown(int count);

	AI* getBot(uint16_t id);

	void sendGameData(Room& room);
};

#endif