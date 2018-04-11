#ifndef PACKETCOMPRESS_H
#define PACKETCOMPRESS_H

#include <SFML/Network.hpp>
#include <deque>

class HistoryState;
class AI;

class PacketCompress {
public:
	std::deque<uint8_t> tmp;
	uint8_t bitcount=0;
	uint16_t tmpcount=0;

	void extract(HistoryState&);
	void getBits(uint8_t&, uint8_t);

	void compress(AI& ai);
	void addBits(uint8_t, uint8_t);

	void loadTmp(sf::Packet&);
	void dumpTmp(sf::Packet&);
};

#endif