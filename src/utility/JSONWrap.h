#ifndef JSONWRAP_H
#define JSONWRAP_H

#include <SFML/Network.hpp>
#include <list>

class Client;
class StatsHolder;

struct Pair {
	std::string key, value;
};

class JSONWrap {
public:
	std::list<Pair> pairs;

	void addPair(const std::string& key, const std::string& value, bool wrapinquotes=true);
	void addPair(const std::string& key, int64_t value);
	void addStatsTable(Client& client, const std::string& table_name);
	std::string getJsonString();
	void jsonToClientStats(StatsHolder& stats, std::string jsonString);
	sf::Http::Response sendPost(const std::string& _request, const std::string& _url="http://localhost", const std::string& contenttype="application/x-www-form-urlencoded");
	void clear();
};

#endif