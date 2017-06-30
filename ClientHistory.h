#ifndef CLIENTHISTORY_H
#define CLIENTHISTORY_H

#include <SFML/Network.hpp>
#include <list>

class Client;

class HistoryState {
public:
	sf::Uint8 square[22][10];
	sf::Uint8 piece, nextpiece, combo, pending, bpm, comboTimer, countdown;
	sf::Uint16 time;
};

class PlayfieldHistory {
public:
	PlayfieldHistory(Client&);
	Client& client;
	std::list<HistoryState> states;

	sf::Int8 lastTimeDiff;
	sf::Int8 maxTimeDiff;

	sf::Int8 timeDiffDirectionCount;

	void validate();

	void clear();
};

#endif