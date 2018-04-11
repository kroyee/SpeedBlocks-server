#include "AIManager.h"
#include "Room.h"
#include <iostream>

static std::vector<std::string> names = {
	"superBot",
	"r2speed2",
	"noeltron",
	"explBoT",
	"A20",
	"MooseMasher",
	"YLYL",
	"emc2",
	"Speed",
	"Blocks"
};

AI& AIManager::add(Room& room) {
	static uint16_t idcount = 59000;
	bots.emplace_back(room);
	AI& bot = bots.back();
	bot.id = idcount++;
	bot.setSpeed(70);
	bot.name = names[bot.rander.get() * names.size()];

	if (idcount > 59999)
		idcount = 59000;

	room.join(bots.back());

	terminateThread=false;
	if (!aiThread.joinable())
		aiThread = std::thread(&AIManager::threadRun, this);

	return bot;
}

void AIManager::clear() {
	terminateThread=true;
	for (auto& bot : bots)
		bot.room->leave(bot);
	bots.clear();
	if (aiThread.joinable())
		aiThread.join();
}

void AIManager::threadRun() {
	while (!terminateThread) {
		for (auto& bot : bots)
			if (bot.alive)
				if (bot.aiThreadRun())
					bot.updateGameData();
		sf::sleep(sf::seconds(0));
	}
}

std::vector<std::pair<uint16_t, std::string>> AIManager::getBots() {
	std::vector<std::pair<uint16_t, std::string>> bot_list;
	for (auto& bot : bots)
		bot_list.push_back({bot.id, bot.name});

	return bot_list;
}
