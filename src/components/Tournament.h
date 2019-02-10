#ifndef TOURNAMENT_H
#define TOURNAMENT_H

#include <SFML/Network.hpp>
#include <list>
#include <thread>
#include "NetworkPackets.hpp"

class HumanClient;
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

    template <class F>
    auto serialize(F f) {
        std::vector<TournamentScore> scores;
        scores.push_back({p1_sets, p1_rounds});
        scores.push_back({p2_sets, p2_rounds});
        return f(scores);
    }
};

class Participant {
   public:
    Participant();
    uint16_t id;
    std::string name;
    sf::Time waitingTime;
    uint16_t sentWaitingTime;
    bool played;
    uint8_t position;

    template <class F>
    auto serialize(F f) {
        return f(id, name);
    }
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
    void sendResults(bool asPart = false);
    void sendScore();
    void decideGame();
    void sendReadyAlert();
    void sendWaitTime(uint16_t waitTime, uint8_t player);
    void resetWaitTimeSent();

    template <class F>
    auto serialize(F f) {
        std::vector<ClientInfo> players;
        auto player_info = [&](Participant* p, Node* g) {
            if (p)
                players.push_back({p->id, p->name});
            else if (g)
                players.push_back({g->id, ""});
            else
                players.push_back({0, ""});
        };

        player_info(player1, p1game);
        player_info(player2, p2game);

        return f(id, depth, status, players, result);
    }
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
    std::list<HumanClient*> keepUpdated;
    Bracket bracket;
    uint16_t players;
    uint8_t rounds, sets;
    uint64_t startingTime;
    std::vector<uint16_t> moderator_list;
    uint8_t status;
    sf::Time waitingTime;

    std::string name;
    uint16_t id;
    uint8_t grade;

    PM packet;

    std::thread* thread;
    bool scoreSent, scoreSentFailed, updated, notify;

    template <class F>
    auto serialize(F f) {
        return f(id, grade, rounds, sets, startingTime, participants, moderator_list, status, bracket.games);
    }

    bool addPlayer(HumanClient& client);
    bool addPlayer(const std::string& name, uint16_t id);
    bool removePlayer(uint16_t id);
    void addObserver(HumanClient& client);
    void removeObserver(HumanClient& client);
    void setStartingTime(short days, short hours, short minutes);

    void makeBracket();
    void linkGames(Node& game1, Node& game2);
    void putPlayersInBracket();
    void setGameStatus();
    void collapseBracket();
    void printBracket();
    void sendTournament();
    void sendParticipantList(bool asPart = false);
    void sendModeratorList(bool asPart = false);
    void sendStatus(bool asPart = false);
    void sendGames(bool asPart = false);

    void sendToTournamentObservers();

    void startTournament();

    void checkIfStart();
    void checkWaitTime();
    void checkIfScoreWasSent();

    void scoreTournament();

    void save();
};

#endif
