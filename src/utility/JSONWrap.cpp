#include "JSONWrap.h"
#include "Client.h"
#include "Connections.h"
#include <iostream>
using std::cout;
using std::endl;
using std::to_string;

void JSONWrap::addPair(const sf::String& key, const sf::String& value, bool wrapinquotes) {
	Pair newpair;
	newpair.key = key;
	if (wrapinquotes)
		newpair.value = "\"" + value + "\"";
	else
		newpair.value = value;
	pairs.push_back(newpair);
}

void JSONWrap::addPair(const sf::String& key, uint32_t value) {
	Pair newpair;
	newpair.key = key;
	newpair.value = to_string(value);
	pairs.push_back(newpair);
}

/*void JSONWrap::add1v1Stats(Client& client) {
	clear();
	addPair("key", client.conn->serverkey);
	addPair("user_id", client.id);
	addPair("table_name", "1v1");
	addPair("points", client.stats.vsPoints);
	addPair("rank", client.stats.vsRank);
	addPair("played", client.stats.vsPlayed);
	addPair("won", client.stats.vsWon);
}

void JSONWrap::addFFAStats(Client& client) {
	clear();
	addPair("key", client.conn->serverkey);
	addPair("user_id", client.id);
	addPair("table_name", "ffa");
	addPair("points", client.stats.ffaPoints);
	addPair("rank", client.stats.ffaRank);
	addPair("played", client.stats.ffaPlayed);
	addPair("won", client.stats.ffaWon);
}

void JSONWrap::addHeroStats(Client& client) {
	clear();
	addPair("key", client.conn->serverkey);
	addPair("user_id", client.id);
	addPair("table_name", "hero");
	addPair("points", client.stats.heroPoints);
	addPair("rank", client.stats.heroRank);
	addPair("played", client.stats.heroPlayed);
	addPair("won", client.stats.heroWon);
}

void JSONWrap::addTournamentStats(Client& client) {
	clear();
	addPair("key", client.conn->serverkey);
	addPair("user_id", client.id);
	addPair("table_name", "tstats");
	addPair("gradeA", client.stats.gradeA);
	addPair("gradeB", client.stats.gradeB);
	addPair("gradeC", client.stats.gradeC);
	addPair("gradeD", client.stats.gradeD);
	addPair("played", client.stats.tournamentsPlayed);
	addPair("won", client.stats.tournamentsWon);
}

void JSONWrap::addGeneralStats(Client& client) {
	clear();
	addPair("key", client.conn->serverkey);
	addPair("user_id", client.id);
	addPair("table_name", "gstats");
	addPair("maxcombo", client.stats.maxCombo);
	addPair("maxbpm", client.stats.maxBpm);
	addPair("alert", client.stats.alert);
	addPair("avgbpm", client.stats.avgBpm);
	addPair("played", client.stats.totalPlayed);
	addPair("won", client.stats.totalWon);
	addPair("totalbpm", client.stats.totalBpm);
	addPair("challenges_played", client.stats.challenges_played);
}*/

void JSONWrap::addStatsTable(Client& client, const std::string& table_name) {
	clear();
	addPair("key", client.conn->serverkey);
	addPair("user_id", client.id);
	addPair("table_name", table_name);

	for (auto& pair : client.stats.get(table_name))
		addPair(pair.first, pair.second);
}

std::string JSONWrap::getJsonString() {
	std::string jsonString;
	auto ending = pairs.end();
	ending--;
	for (auto it = pairs.begin(); it != pairs.end(); it++) {
		if (it == pairs.begin())
			jsonString = "{";
		jsonString += "\"" + it->key + "\":" + it->value;
		if (it == ending)
			jsonString += "}";
		else
			jsonString += ",";
	}
	return jsonString;
}

void JSONWrap::jsonToClientStats(StatsHolder& stats, std::string jsonString) {
	std::size_t start=0, stop;
	if (jsonString.size() < 3)
		return;
	jsonString.erase(jsonString.begin());
	jsonString.erase(jsonString.end()-1);
	while (true) {
		stop = jsonString.find(":", start);
		if (stop == std::string::npos)
			break;
		std::string key = jsonString.substr(start+1, stop-start-2);
		start = stop+1;
		stop = jsonString.find(",", start);
		uint32_t value = stol(jsonString.substr(start, stop-start));
		start = stop+1;
		stats.set(key, value);
		if (stop == std::string::npos)
			break;
	}
}

sf::Http::Response JSONWrap::sendPost(const sf::String& _request, const sf::String& _url, const sf::String& contenttype) {
	sf::Http::Request request(_request, sf::Http::Request::Post);
	request.setBody(getJsonString());
    request.setField("Content-Type", contenttype);
	sf::Http http(_url);
    return http.sendRequest(request);
}

void JSONWrap::clear() {
	pairs.clear();
}