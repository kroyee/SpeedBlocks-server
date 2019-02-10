#ifndef PINGHANDLE_H
#define PINGHANDLE_H

#include <SFML/Network.hpp>
#include <deque>

class HumanClient;
struct NP_Ping;

class PingHandle {
   private:
    struct PingPacket {
        uint8_t id;
        sf::Time sent, received;
        uint16_t ping;
        bool returned = false;
    };

    std::deque<PingPacket> packets;
    uint8_t pingIdCount = 0, packetCount = 0;
    sf::Time lastSend = sf::seconds(0);

   public:
    void get(const sf::Time& t, HumanClient& client, const NP_Ping& p);
    int getAverage();
    float getPacketLoss(const sf::Time& t);
    int getLowest();
};

#endif
