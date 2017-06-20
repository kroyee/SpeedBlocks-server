#ifndef REPLAY_H
#define REPLAY_H

#include <SFML/Network.hpp>
#include <list>

class Client;
class Connections;

struct Replay {
	sf::Uint16 id;
	sf::Packet packet;

	void save(std::string filename="");
	bool load(std::string filename);

	void sendRecording(Client&);
	void receiveRecording(sf::Packet&);
};

#endif