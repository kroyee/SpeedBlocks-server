#ifndef TOURNAMENT_H
#define TOURNAMENT_H

#include <SFML/Network.hpp>
#include <list>

class Client;

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
	void addGame(short _depth);
};

class Tournament {
public:
	Tournament();
	std::list<Participant> participants;
	Bracket bracket;
	short players, rounds, sets;

	sf::String name;

	bool signupOpen, active;

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