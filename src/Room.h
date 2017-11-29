#ifndef ROOM_H
#define ROOM_H

#include <SFML/Network.hpp>
#include "Client.h"
#include "EloResults.h"

class Connections;
class Node;
class Tournament;

class Adjust {
public:
	short players;
	short amount;
};

class Room {
public:
	Room(Connections& _conn, sf::Uint16 _gamemode);
	virtual ~Room() = default;
	Connections& conn;
	sf::String name;
	sf::Uint16 id;

	sf::Uint8 maxPlayers;
	sf::Uint8 currentPlayers;
	sf::Uint8 activePlayers;

	std::list<Client*> clients;
	std::list<Client*> spectators;

	std::list<Client> leavers;

	std::list<Adjust> adjust;

	//Room setting
	sf::Clock start;
	bool active;
	bool round, endround, waitForReplay, locked, countdown;
	sf::Time roundLenght, timeBetweenRounds;
	sf::Uint8 playersAlive;
	sf::Uint16 seed1, seed2;

	const sf::Uint16 gamemode; // 1=Ranked FFA, 2=Ranked hero, 3=Unranked FFA, 4=Tournament, 5=1v1, 20000+=challenge
	Node* tournamentGame;
	Tournament* tournament;

	EloResults eloResults;

	sf::Time countdownTime;

	void join(Client&);
	void leave(Client&);
	void matchLeaver(Client&);

	bool addSpectator(Client&);
	void removeSpectator(Client&);

	void startGame();
	void endRound();

	void scoreFFARound();
	void scoreTournamentRound();
	void score1vs1Round();
	void scoreHeroRound();

	void transfearScore();
	void updatePlayerScore();
	void playerDied(Client&);
	void setInactive();
	void setActive();
	void lock();
	void sendGameData();
	void makeCountdown();
	void checkIfRoundEnded();
	void startCountdown();

	void sendLines(Client& client);
	void sendNewPlayerInfo(Client& client);
	void sendRoundScores();

	void sendSignal(sf::Uint8 signalId, int id1 = -1, int id2 = -1);
	void sendSignalToAway(sf::Uint8 signalId, int id1 = -1, int id2 = 1);
	void sendSignalToActive(sf::Uint8 signalId, int id1 = -1, int id2 = -1);
	void sendSignalToSpectators(sf::Uint8 signalId, int id1 = -1, int id2 = -1);

	void sendPacket();
	void sendPacketToPlayers();
	void sendPacketToSpectators();

	virtual void scoreRound() {}
	virtual void incrementGamesPlayed(Client&) = 0;
	virtual void incrementGamesWon(Client&) {}
};

class FFARoom : public Room {
	void scoreRound();
	void incrementGamesPlayed(Client& client) { ++client.stats.ffaPlayed; ++client.stats.totalPlayed; }
	void incrementGamesWon(Client& client) { ++client.stats.ffaWon; ++client.stats.totalWon; }
public:
	FFARoom(Connections& _conn) : Room(_conn, 1) {}
};

class HeroRoom : public Room {
	void scoreRound();
	void incrementGamesPlayed(Client& client) { ++client.stats.heroPlayed; ++client.stats.totalPlayed; }
	void incrementGamesWon(Client& client) { ++client.stats.heroWon; ++client.stats.totalWon; }
public:
	HeroRoom(Connections& _conn) : Room(_conn, 2) {}
};

class CasualRoom : public Room {
	void incrementGamesPlayed(Client& client) { ++client.stats.totalPlayed; }
	void incrementGamesWon(Client& client) { ++client.stats.totalWon; }
public:
	CasualRoom(Connections& _conn) : Room(_conn, 3) {}
};

class TournamentRoom : public Room {
	void scoreRound();
	void incrementGamesPlayed(Client& client) { ++client.stats.vsPlayed; ++client.stats.totalPlayed; }
	void incrementGamesWon(Client& client) { ++client.stats.vsWon; ++client.stats.totalWon; }
public:
	TournamentRoom(Connections& _conn) : Room(_conn, 4) {}
};

class VSRoom : public Room {
	void scoreRound();
	void incrementGamesPlayed(Client& client) { ++client.stats.vsPlayed; ++client.stats.totalPlayed; }
	void incrementGamesWon(Client& client) { ++client.stats.vsWon; ++client.stats.totalWon; }
public:
	VSRoom(Connections& _conn) : Room(_conn, 5) {}
};

class ChallengeRoom : public Room {
	void incrementGamesPlayed(Client& client) { ++client.stats.challenges_played; }
public:
	ChallengeRoom(Connections& _conn, sf::Uint16 mode) : Room(_conn, mode) {}
};

#endif