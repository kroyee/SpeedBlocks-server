#ifndef ELORESULTS_H
#define ELORESULTS_H

#include <SFML/Network.hpp>
#include <list>

class Client;

struct Result {
	Client *winner, *loser;
	float winner_expected, loser_expected;
	uint8_t type;
};

class EloResults {
public:
	std::list<Result> result;

	void addResult(Client& winner, Client& loser, uint8_t type);
	void calculateResults();
};

#endif