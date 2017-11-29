#ifndef ELORESULTS_H
#define ELORESULTS_H

#include <SFML/Network.hpp>
#include <list>

class Client;

struct Result {
	Client *winner, *loser;
	float winner_expected, loser_expected;
	sf::Uint8 type;
};

class EloResults {
public:
	std::list<Result> result;

	void addResult(Client& winner, Client& loser, sf::Uint8 type);
	void calculateResults();
};

#endif