#ifndef PACKETCOMPRESS_H
#define PACKETCOMPRESS_H

#include <SFML/Network.hpp>

class PlayfieldHistory;

class PacketCompress {
public:
	sf::Uint8 tmp[100];
	sf::Uint8 bitcount=0;
	sf::Uint8 tmpcount=0;

	void extract(PlayfieldHistory&);
	void getBits(sf::Uint8&, sf::Uint8);
};

#endif