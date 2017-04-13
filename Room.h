#ifndef ROOM_H
#define ROOM_H

#include <SFML/Network.hpp>
#include "Client.h"

class Connections;

class Adjust {
public:
	short players;
	short amount;
};

class Room {
public:
	Room(Connections* _conn) : conn(_conn), active(false), round(false) {}
	Connections* conn;
	sf::String name;
	sf::Uint16 id;

	sf::Uint8 maxPlayers;
	sf::Uint8 currentPlayers;
	sf::Uint8 activePlayers;

	std::list<Client*> clients;

	std::list<Client> leavers;

	std::list<Adjust> adjust;

	//Room setting
	sf::Clock start;
	bool active;
	short countdownSetting;
	short countdown;
	bool round, endround;
	sf::Time countdownTime, roundLenght;
	sf::Uint8 playersAlive;
	sf::Uint16 seed1, seed2;

	sf::Uint8 gamemode; // 1=Ranked FFA, 2=Ranked hero, 3=Unranked FFA

	void join(Client&);
	void leave(Client&);
	void startGame();
	void endRound();
	void scoreFFARound();
	void transfearScore();
	void updatePlayerScore();
	void playerDied();
	void setInactive();
	void setActive();
	void sendGameData();
	void makeCountdown();
	void checkIfRoundEnded();
};

#endif