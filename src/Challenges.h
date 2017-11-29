#ifndef CHALLENGES_H
#define CHALLENGES_H

#include <SFML/Network.hpp>
#include <list>
#include "Replay.h"
#include <memory>

class Connections;
class Client;
class Replay;

struct Column {
	Column(sf::Uint16 _width, sf::String _text) : width(_width), text(_text) {}
	sf::Uint16 width;
	sf::String text;
};

struct Score {
	sf::Uint16 id;
	sf::String name;

	std::vector<sf::String> column;
	sf::Uint32 duration;

	Replay replay;

	bool update;
};

class Challenge {
public:
	sf::String name;
	sf::Uint16 id;
	sf::String label;

	std::vector<Column> columns;
	std::list<Score> scores;
	sf::Uint16 scoreCount=0;

	bool update;

	void addScore(Client& client, sf::Uint32 duration, sf::Uint16 blocks);
	virtual void setColumns(Client&, sf::Uint16 blocks, Score& score) = 0;
	virtual bool sort(Score&, Score&);
	virtual bool checkResult(Client& client, sf::Uint32 duration, sf::Uint16 blocks, Score& score) = 0;
	void loadScores();
};

class ChallengeHolder {
public:
	ChallengeHolder(Connections& _conn);

	Connections& conn;
	std::vector<std::unique_ptr<Challenge>> challenges;

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

////////////////// Challenge instances //////////////////////

class CH_Race : public Challenge {
public:
	CH_Race();
	void setColumns(Client&, sf::Uint16 blocks, Score& score);
	bool checkResult(Client& client, sf::Uint32 duration, sf::Uint16 blocks, Score& score);
};

class CH_Cheese : public Challenge {
public:
	CH_Cheese();
	void setColumns(Client&, sf::Uint16 blocks, Score& score);
	bool checkResult(Client& client, sf::Uint32 duration, sf::Uint16 blocks, Score& score);
};

class CH_Survivor : public Challenge {
public:
	CH_Survivor();
	void setColumns(Client&, sf::Uint16 blocks, Score& score);
	bool checkResult(Client& client, sf::Uint32 duration, sf::Uint16 blocks, Score& score);
	bool sort(Score& score1, Score& score2);
};

class CH_Cheese30L : public Challenge {
public:
	CH_Cheese30L();
	void setColumns(Client&, sf::Uint16 blocks, Score& score);
	bool checkResult(Client& client, sf::Uint32 duration, sf::Uint16 blocks, Score& score);
};

#endif