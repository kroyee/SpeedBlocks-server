#ifndef JSONWRAP_H
#define JSONWRAP_H

#include <SFML/Network.hpp>
#include <list>

class Client;
class StatsHolder;

struct Pair {
	sf::String key, value;
};

class JSONWrap {
public:
	std::list<Pair> pairs;

	void addPair(const sf::String& key, const sf::String& value, bool wrapinquotes=true);
	void addPair(const sf::String& key, sf::Uint32 value);
	void add1v1Stats(Client& client);
	void addFFAStats(Client& client);
	void addHeroStats(Client& client);
	void addTournamentStats(Client& client);
	void addGeneralStats(Client& client);
	std::string getJsonString();
	void jsonToClientStats(StatsHolder& stats, std::string jsonString);
	sf::Http::Response sendPost(const sf::String& _request, const sf::String& _url="http://localhost", const sf::String& contenttype="application/x-www-form-urlencoded");
	void clear();
};

#endif