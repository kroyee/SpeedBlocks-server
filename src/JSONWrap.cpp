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

void JSONWrap::addPair(const sf::String& key, sf::Uint32 value) {
	Pair newpair;
	newpair.key = key;
	newpair.value = to_string(value);
	pairs.push_back(newpair);
}

void JSONWrap::add1v1Stats(Client& client) {
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

#define MAP_VARIABLE(KEY,VARIABLE) if (key == #KEY) stats.VARIABLE = value

void mapStringToVariable(StatsHolder& stats, std::string key, sf::Uint32 value) {
	// General
	MAP_VARIABLE(gstatsmaxcombo, maxCombo);
	else MAP_VARIABLE(gstatsmaxbpm, maxBpm);
	else MAP_VARIABLE(gstatsavgbpm, avgBpm);
	else MAP_VARIABLE(gstatswon, totalWon);
	else MAP_VARIABLE(gstatsplayed, totalPlayed);
	else MAP_VARIABLE(gstatstotalbpm, totalBpm);
	else MAP_VARIABLE(gstatsalert, alert);
	else MAP_VARIABLE(gstatschallenges_played, challenges_played);

	// 1v1
	else MAP_VARIABLE(1v1rank, vsRank);
	else MAP_VARIABLE(1v1points, vsPoints);
	else MAP_VARIABLE(1v1played, vsPlayed);
	else MAP_VARIABLE(1v1won, vsWon);

	// Hero
	else MAP_VARIABLE(heropoints, heroPoints);
	else MAP_VARIABLE(herorank, heroRank);
	else MAP_VARIABLE(heroplayed, heroPlayed);
	else MAP_VARIABLE(herowon, heroWon);

	// FFA
	else MAP_VARIABLE(ffapoints, ffaPoints);
	else MAP_VARIABLE(ffarank, ffaRank);
	else MAP_VARIABLE(ffaplayed, ffaPlayed);
	else MAP_VARIABLE(ffawon, ffaWon);
	
	// Tournament
	else MAP_VARIABLE(tstatsplayed, tournamentsPlayed);
	else MAP_VARIABLE(tstatswon, tournamentsWon);
	else MAP_VARIABLE(tstatsgradeA, gradeA);
	else MAP_VARIABLE(tstatsgradeB, gradeB);
	else MAP_VARIABLE(tstatsgradeC, gradeC);
	else MAP_VARIABLE(tstatsgradeD, gradeD);
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
		sf::Uint32 value = stol(jsonString.substr(start, stop-start));
		start = stop+1;
		mapStringToVariable(stats, key, value);
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