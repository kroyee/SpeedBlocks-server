#ifndef JSONWRAP_H
#define JSONWRAP_H

#include <SFML/Network.hpp>
#include <list>

class Client;

struct Pair {
	sf::String key, value;
};

class JSONWrap {
public:
	std::list<Pair> pairs;

	void addPair(const sf::String& key, const sf::String& value, bool wrapinquotes=true);
	void addPair(const sf::String& key, sf::Uint32 value);
	void addClientStats(Client& client);
	std::string getJsonString();
	void jsonToClientStats(Client& client, std::string jsonString);
	sf::Http::Response sendPost(const sf::String& _request);
	void clear();
};

#endif