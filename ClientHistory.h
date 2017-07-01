#ifndef CLIENTHISTORY_H
#define CLIENTHISTORY_H

#include <SFML/Network.hpp>
#include <list>

class Client;

class HistoryState {
public:
	sf::Uint8 square[22][10];
	sf::Uint8 piece, nextpiece, combo, pending, bpm, comboTimer, countdown;
	sf::Uint32 time;
	sf::Uint32 received;
};

class PlayfieldHistory {
public:
	PlayfieldHistory(Client&);
	Client& client;
	std::list<HistoryState> states;

	sf::Int16 lastTimeDiff;
	sf::Int16 maxTimeDiff;

	sf::Int16 timeDiffDirectionCount;
	sf::Uint32 lastEveningOutDirectionCount;

	void validate();

	void clear();
};

#endif