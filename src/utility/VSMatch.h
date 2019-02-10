#ifndef VSMATCH_H
#define VSMATCH_H

#include <SFML/Network.hpp>
#include <list>
#include "NetworkPackets.hpp"

class Client;

struct QueueClient {
    Client *client;
    sf::Time time;
    uint16_t lastOpponent;
};

class VSMatch {
   public:
    std::list<QueueClient> queue;
    std::list<QueueClient> playing;

    Client *player1, *player2;

    void addToQueue(Client &, const sf::Time &);
    void removeFromQueue(Client &);

    void setPlaying();
    void setQueueing(Client &, const sf::Time &);

    bool checkQueue(const sf::Time &);

    template <class F>
    auto serialize(F f) {
        NP_MatchMaking p{static_cast<uint16_t>(queue.size()), static_cast<uint16_t>(playing.size())};
        return f(p);
    }
};

#endif