#ifndef CHALLENGES_H
#define CHALLENGES_H

#include <SFML/Network.hpp>
#include <list>
#include <memory>
#include "NetworkPackets.hpp"
#include "json.hpp"

class Connections;
class Client;
struct NP_JoinChallenge;

struct Column {
    Column(uint16_t _width, std::string _text) : width(_width), text(_text) {}
    uint16_t width;
    std::string text;
};

struct Score {
    uint16_t id;
    std::string name;

    std::vector<std::string> column;
    uint32_t duration;

    NP_Replay replay;

    bool update = false;

    template <class F>
    auto serialize(F f) {
        return f(name, column);
    }
};

class Challenge {
   public:
    std::string name;
    uint16_t id;
    std::string label;

    std::vector<Column> columns;
    std::list<Score> scores;
    uint16_t scoreCount = 0;

    bool update;

    void addScore(Client& client, uint32_t duration, uint16_t blocks);
    virtual void setColumns(Client&, uint16_t blocks, Score& score) = 0;
    virtual bool sort(Score&, Score&);
    virtual bool checkResult(Client& client, uint32_t duration, uint16_t blocks, Score& score) = 0;
    void loadScores(nlohmann::json& json);
    void sendScores(std::string serverkey);

    template <class F>
    auto serialize(F f) {
        return f(id, columns, scores);
    }
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
    void sendLeaderboard(uint16_t);

    void checkResult(Client&);
    void sendReplayRequest(Client& client, std::string text);
    void sendNotGoodEnough(Client& client, std::string text);
    void updateResult(Client& client, const NP_Replay&);

    void sendReplay(const NP_ChallengeWatchReplay&);
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