#include "AIManager.h"

AIManager::AIManager(sf::Clock& _gameclock, Signal<void, uint16_t, RoundStats&, uint16_t>& _sendLines) :
gameclock(_gameclock),
botSendLines(_sendLines) {}

void AIManager::setAmount(unsigned int amount) {
	//std::lock_guard<std::mutex> mute(listMutex);
	while (amount > bots.size())
		bots.emplace_back(gameclock);

	while (amount < bots.size())
		bots.pop_back();
}

void AIManager::threadRun() {
	while (terminateThread) {
		for (auto& bot : bots)
			if (bot.alive)
				bot.aiThreadRun();

		sf::sleep(sf::seconds(0));
	}
}

void AIManager::sendLines(uint16_t senderid, float amount) {
	//std::lock_guard<std::mutex>> mute(listMutex);
	for (auto& bot : bots)
		if (bot.id != senderid)
			bot.addGarbage(amount, gameclock.getElapsedTime());
}

static auto& SeedRander = Signal<void, uint16_t, uint16_t>::get("SeedRander");
void AIManager::startCountdown(uint16_t seed1, uint16_t seed2) {
	SeedRander(seed1, seed2);
	for (auto& bot : bots)
		bot.startCountdown();
}

void AIManager::countDown(int count) {
	for (auto& bot : bots)
		bot.countDown(count);
}

void AIManager::startRound() {
	for (auto& bot : bots)
		bot.startRound();

	alive = bots.size();
	terminateThread = false;
	if (alive)
		aiThread = std::thread(&AIManager::threadRun, this);
}

void AIManager::endRound(const sf::Time& t) {
	for (auto& bot : bots)
		if (bot.alive)
			bot.endRound(t, true);
	terminateThread = true;
	if (aiThread.joinable())
		aiThread.join();
}

AI* AIManager::getBot(uint16_t id) {
	for (auto& bot : bots)
		if (bot.id == id)
			return &bot;
	return nullptr;
}