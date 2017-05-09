#ifndef TOURNAMENT_H
#define TOURNAMENT_H

#include <SFML/Network.hpp>
#include <list>

class Client;

class Results {
public:
	Results();
	std::vector<sf::Uint8> set;
	std::vector<sf::Uint8> round;
};

class Participant {
public:
	Participant();
	sf::Uint16 id;
	sf::String name;
};

class Node {
public:
	Node();
	Participant *player1, *player2;
	Node *p1game, *p2game, *nextgame;
	short depth;
	sf::Uint16 id;
	Results result;
	std::vector<sf::Packet> p1Replays, p2Replays;
	time_t startingTime;
};

class Bracket {
public:
	Bracket();
	std::list<Node> games;
	short players;
	short depth;
	short gameCount;
	sf::Uint16 idCount;

	void clear();
	void addGame(short _depth, sf::Uint8 sets);
};

class Tournament {
public:
	Tournament();
	std::list<Participant> participants;
	Bracket bracket;
	sf::Uint16 players;
	sf::Uint8 rounds, sets;
	time_t startingTime;
	std::vector<sf::Uint16> moderator_list;

	sf::String name;

	bool signupOpen, active, useStartingTime, useGameStartingTime;

	bool addPlayer(Client& client);
	bool addPlayer(const sf::String& name, sf::Uint16 id);
	bool removePlayer(sf::Uint16 id);

	void makeBracket();
	void linkGames(Node& game1, Node& game2);
	void putPlayersInBracket();
	void collapseBracket();
	void printBracket();
};

#endif