#ifndef CHALLENGES_H
#define CHALLENGES_H

#include <SFML/Network.hpp>
#include <list>
#include "Replay.h"
#include "json.hpp"
#include <memory>

class Connections;
class Client;
class Replay;

struct Column {
	Column(uint16_t _width, sf::String _text) : width(_width), text(_text) {}
	uint16_t width;
	sf::String text;
};

struct Score {
	uint16_t id;
	sf::String name;

	std::vector<sf::String> column;
	uint32_t duration;

	Replay replay;

	bool update;
};

class Challenge {
public:
	sf::String name;
	uint16_t id;
	sf::String label;

	std::vector<Column> columns;
	std::list<Score> scores;
	uint16_t scoreCount=0;

	bool update;

	void addScore(Client& client, uint32_t duration, uint16_t blocks);
	virtual void setColumns(Client&, uint16_t blocks, Score& score) = 0;
	virtual bool sort(Score&, Score&);
	virtual bool checkResult(Client& client, uint32_t duration, uint16_t blocks, Score& score) = 0;
	void loadScores(nlohmann::json json);
	void sendScores(sf::String serverkey);
};

class ChallengeHolder {
public:
	ChallengeHolder(Connections& _conn);

	Connections& conn;
	std::vector<std::unique_ptr<Challenge>> challenges;

	uint8_t challengeCount;
	uint16_t idcount;

	sf::Time updateTime;

	void clear();

	void saveChallenges();
	void loadChallenges();
	void getChallengeScores();

	void addChallenge();

	void sendChallengeList(Client&);
	void sendChallengeReplay();
	void sendLeaderboard(uint16_t id);

	void checkResult(Client&, sf::Packet&);
	void sendReplayRequest(Client& client, sf::String text);
	void sendNotGoodEnough(Client& client, sf::String text);
	void updateResult(Client& client, uint16_t id, sf::Packet& packet);

	void sendReplay(uint16_t id, uint16_t slot);
};

////////////////// Challenge instances //////////////////////

class CH_Race : public Challenge {
public:
	CH_Race();
	void setColumns(Client&, uint16_t blocks, Score& score);
	bool checkResult(Client& client, uint32_t duration, uint16_t blocks, Score& score);
};

class CH_Cheese : public Challenge {
public:
	CH_Cheese();
	void setColumns(Client&, uint16_t blocks, Score& score);
	bool checkResult(Client& client, uint32_t duration, uint16_t blocks, Score& score);
};

class CH_Survivor : public Challenge {
public:
	CH_Survivor();
	void setColumns(Client&, uint16_t blocks, Score& score);
	bool checkResult(Client& client, uint32_t duration, uint16_t blocks, Score& score);
	bool sort(Score& score1, Score& score2);
};

class CH_Cheese30L : public Challenge {
public:
	CH_Cheese30L();
	void setColumns(Client&, uint16_t blocks, Score& score);
	bool checkResult(Client& client, uint32_t duration, uint16_t blocks, Score& score);
};

#endif