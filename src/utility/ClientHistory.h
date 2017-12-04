#ifndef CLIENTHISTORY_H
#define CLIENTHISTORY_H

#include <SFML/Network.hpp>
#include <list>

class Client;

class HistoryState {
public:
	uint8_t square[22][10];
	uint8_t piece, nextpiece, combo, pending, bpm, comboTimer, countdown;
	uint32_t time;
	uint32_t received;
};

class PlayfieldHistory {
public:
	PlayfieldHistory();
	Client* client;
	std::list<HistoryState> states;

	sf::Int16 lastTimeDiff;
	sf::Int16 maxTimeDiff;

	sf::Int16 timeDiffDirectionCount;
	uint32_t lastEveningOutDirectionCount;

	void validate();

	void clear();
};

#endif