#ifndef VSMATCH_H
#define VSMATCH_H

#include <SFML/Network.hpp>
#include <list>

class Client;

struct QueueClient {
	Client *client;
	sf::Time time;
	sf::Uint16 lastOpponent;
};

class VSMatch {
public:
	std::list<QueueClient> queue;
	std::list<QueueClient> playing;

	Client *player1, *player2;

	void addToQueue(Client&, const sf::Time&);
	void removeFromQueue(Client&);

	void setPlaying();
	void setQueueing(Client&, const sf::Time&);

	bool checkQueue(const sf::Time&);
};

#endif