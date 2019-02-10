#ifndef CLIENTHISTORY_H
#define CLIENTHISTORY_H

#include <SFML/Network.hpp>
#include <array>
#include <list>
#include "PacketCompressServer.h"

class Client;

class HistoryState {
   public:
    std::array<std::array<uint8_t, 10>, 22> square;
    uint8_t piece, nextpiece, combo, pending, bpm, comboTimer, countdown;
    uint32_t time;
    uint32_t received;

    HistoryState& operator=(const PacketCompressServer&);
};

class PlayfieldHistory {
   public:
    PlayfieldHistory();
    Client* client;
    std::list<HistoryState> states;

    sf::Int16 lastTimeDiff;
    sf::Int16 maxTimeDiff;

    sf::Int16 timeDiffDirectionCount;
    uint32_t lastEveningOutDirectionCount;

    void validate();

    void clear();
};

#endif