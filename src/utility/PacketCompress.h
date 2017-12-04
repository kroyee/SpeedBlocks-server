#ifndef PACKETCOMPRESS_H
#define PACKETCOMPRESS_H

#include <SFML/Network.hpp>

class HistoryState;

class PacketCompress {
public:
	uint8_t tmp[100];
	uint8_t bitcount=0;
	uint8_t tmpcount=0;

	void extract(HistoryState&);
	void getBits(uint8_t&, uint8_t);
};

#endif