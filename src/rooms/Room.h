#ifndef ROOM_H
#define ROOM_H

#include <SFML/Network.hpp>
#include "Client.h"
#include "EloResults.h"
#include "AIManager.h"
#include "LineSendAdjust.h"

class Connections;
class Node;
class Tournament;
struct RoundStats;

class Room {
public:
	Room(Connections& _conn, uint16_t _gamemode);
	virtual ~Room() = default;
	Connections& conn;
	std::string name;
	uint16_t id;

	uint8_t maxPlayers;
	uint8_t currentPlayers = 0;
	uint8_t botCount = 0;
	uint8_t activePlayers = 0;

	std::list<Client*> clients;
	std::list<HumanClient*> spectators;

	std::list<Client> leavers;

	//Room setting
	sf::Clock start;
	bool active;
	bool round, endround, waitForReplay, locked, countdown;
	sf::Time roundLenght, timeBetweenRounds;
	uint8_t playersAlive;
	uint16_t seed1, seed2;

	const uint16_t gamemode; // 1=Ranked FFA, 2=Ranked hero, 3=Unranked FFA, 4=Tournament, 5=1v1, 20000+=challenge
	Node* tournamentGame;
	Tournament* tournament;

	EloResults eloResults;

	sf::Time countdownTime;

	LineSendAdjust lineSendAdjust;

	void join(Client&);
	void leave(Client&);
	void matchLeaver(Client&);

	bool addSpectator(HumanClient&);
	void removeSpectator(Client&);

	void startGame();
	void endRound();

	void score1vs1Round();

	void transfearScore();
	void updatePlayerScore();
	void playerDied(Client&);
	void setInactive();
	void setActive();
	void lock();
	void sendGameData(sf::UdpSocket&);
	void makeCountdown();
	void checkIfRoundEnded();
	void startCountdown();

	void sendLines(Client& client, uint16_t amount);
	void distributeLines(uint16_t senderid, float amount);
	void sendNewPlayerInfo(Client& client);
	void sendRoundScores();

	void sendSignal(uint8_t signalId, int id1 = -1, int id2 = -1);
	void sendSignalToAway(uint8_t signalId, int id1 = -1, int id2 = 1);
	void sendSignalToActive(uint8_t signalId, int id1 = -1, int id2 = -1);
	void sendSignalToSpectators(uint8_t signalId, int id1 = -1, int id2 = -1);

	void sendPacket(sf::Packet& packet);
	void sendPacketToPlayers(sf::Packet& packet);
	void sendPacketToSpectators(sf::Packet& packet);

	bool onlyBots();

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
	ChallengeRoom(Connections& _conn, uint16_t mode) : Room(_conn, mode) {}
};

#endif
