#ifndef PACKETCOMPRESSSERVER_H
#define PACKETCOMPRESSSERVER_H

#include <SFML/Network.hpp>
#include <deque>
#include "PacketCompress.h"

class HistoryState;
class AI;

class PacketCompressServer : public PacketCompress {
   public:
    PacketCompressServer& operator=(const AI&);
};

#endif