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
	uint8_t p1_sets;
	uint8_t p2_sets;

	std::vector<uint8_t> p1_rounds;
	std::vector<uint8_t> p2_rounds;
};

class Participant {
public:
	Participant();
	uint16_t id;
	sf::String name;
	sf::Time waitingTime;
	uint16_t sentWaitingTime;
	bool played;
	uint8_t position;
};

class Node {
public:
	Node(Tournament&);
	Tournament& tournament;
	Participant *player1, *player2;
	Node *p1game, *p2game, *nextgame;
	uint8_t depth, status, sets, rounds, activeSet;
	uint16_t id;
	Results result;
	std::vector<sf::Packet> p1Replays, p2Replays;
	time_t startingTime;
	Room* room;

	bool p1won();
	bool p2won();
	void winByWO(uint8_t player);
	void setPosition();
	void sendResults(bool asPart=false);
	void sendScore();
	void decideGame();
	void sendReadyAlert();
	void sendWaitTime(uint16_t waitTime, uint8_t player);
	void resetWaitTimeSent();
};

class Bracket {
public:
	Bracket();
	std::vector<Node> games;
	short players;
	short depth;
	short gameCount;
	uint16_t idCount;

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
	uint16_t players;
	uint8_t rounds, sets;
	time_t startingTime;
	std::vector<uint16_t> moderator_list;
	uint8_t status;
	sf::Time waitingTime;

	sf::String name;
	uint16_t id;
	uint8_t grade;

	sf::Packet packet;

	std::thread *thread;
	bool scoreSent, scoreSentFailed, updated, notify;

	bool addPlayer(Client& client);
	bool addPlayer(const sf::String& name, uint16_t id);
	bool removePlayer(uint16_t id);
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