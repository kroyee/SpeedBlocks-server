#ifndef TOURNAMENT_H
#define TOURNAMENT_H

#include <SFML/Network.hpp>
#include <list>
#include <thread>

class Client;
class Room;
class Tournament;
class Connections;

class Results {
public:
	Results();
	sf::Uint8 p1_sets;
	sf::Uint8 p2_sets;

	std::vector<sf::Uint8> p1_rounds;
	std::vector<sf::Uint8> p2_rounds;
};

class Participant {
public:
	Participant();
	sf::Uint16 id;
	sf::String name;
	sf::Time waitingTime;
	sf::Uint16 sentWaitingTime;
	bool played;
	sf::Uint8 position;
};

class Node {
public:
	Node(Tournament&);
	Tournament& tournament;
	Participant *player1, *player2;
	Node *p1game, *p2game, *nextgame;
	sf::Uint8 depth, status, sets, rounds, activeSet;
	sf::Uint16 id;
	Results result;
	std::vector<sf::Packet> p1Replays, p2Replays;
	time_t startingTime;
	Room* room;

	bool p1won();
	bool p2won();
	void winByWO(sf::Uint8 player);
	void setPosition();
	void sendResults(bool asPart=false);
	void sendScore();
	void decideGame();
	void sendReadyAlert();
	void sendWaitTime(sf::Uint16 waitTime, sf::Uint8 player);
	void resetWaitTimeSent();
};

class Bracket {
public:
	Bracket();
	std::vector<Node> games;
	short players;
	short depth;
	short gameCount;
	sf::Uint16 idCount;

	void clear();
	void addGame(short _depth, Tournament& tournament);
	void sendAllReadyAlerts();
};

class Tournament {
public:
	Tournament(Connections&);
	Connections& conn;
	std::vector<Participant> participants;
	std::list<Client*> keepUpdated;
	Bracket bracket;
	sf::Uint16 players;
	sf::Uint8 rounds, sets;
	time_t startingTime;
	std::vector<sf::Uint16> moderator_list;
	sf::Uint8 status;
	sf::Time waitingTime;

	sf::String name;
	sf::Uint16 id;
	sf::Uint8 grade;

	std::thread *thread;
	bool scoreSent, scoreSentFailed, updated;

	bool addPlayer(Client& client);
	bool addPlayer(const sf::String& name, sf::Uint16 id);
	bool removePlayer(sf::Uint16 id);
	void addObserver(Client& client);
	void removeObserver(Client& client);
	void setStartingTime(short days, short hours, short minutes);

	void makeBracket();
	void linkGames(Node& game1, Node& game2);
	void putPlayersInBracket();
	void setGameStatus();
	void collapseBracket();
	void printBracket();
	void sendTournament();
	void sendParticipantList(bool asPart=false);
	void sendModeratorList(bool asPart=false);
	void sendStatus(bool asPart=false);
	void sendGames(bool asPart=false);

	void sendToTournamentObservers();

	void startTournament();

	void checkIfStart();
	void checkWaitTime();
	void checkIfScoreWasSent();

	void scoreTournament();

	void save();
};

#endif