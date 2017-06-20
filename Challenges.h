#ifndef CHALLENGES_H
#define CHALLENGES_H

#include <SFML/Network.hpp>
#include <list>
#include "Replay.h"

class Connections;
class Client;
class Replay;

struct Score {
	sf::Uint16 id;
	sf::String name;

	sf::String column[3];
	sf::Uint32 duration;

	Replay replay;

	bool update;
};

struct Challenge {
	sf::String name;
	sf::Uint16 id;
	sf::String label;

	sf::Uint8 columns;
	sf::Uint16 width[4];
	sf::String column[4];

	std::list<Score> scores;

	sf::Uint8 scoreCount;

	bool update;

	void addScore(Client& client, sf::Uint32 duration, sf::Uint16 blocks);
	void loadScores();
};

class ChallengeHolder {
public:
	ChallengeHolder(Connections& _conn);

	Connections& conn;
	std::list<Challenge> challenges;

	sf::Uint8 challengeCount;
	sf::Uint16 idcount;

	sf::Time updateTime;

	void clear();

	void saveChallenges();
	void loadChallenges();

	void addChallenge();

	void sendChallengeList(Client&);
	void sendChallengeReplay();
	void sendLeaderboard(sf::Uint16 id);

	void checkResult(Client&);
	void sendReplayRequest(Client& client, sf::String text);
	void sendNotGoodEnough(Client& client, sf::String text);
	void updateResult(Client& client, sf::Uint16 id);

	void sendReplay();
};

#endif