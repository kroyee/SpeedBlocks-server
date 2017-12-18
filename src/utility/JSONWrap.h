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
	void addPair(const sf::String& key, int64_t value);
	void addStatsTable(Client& client, const std::string& table_name);
	std::string getJsonString();
	void jsonToClientStats(StatsHolder& stats, std::string jsonString);
	sf::Http::Response sendPost(const sf::String& _request, const sf::String& _url="http://localhost", const sf::String& contenttype="application/x-www-form-urlencoded");
	void clear();
};

#endif