#include "JSONWrap.h"
#include "Client.h"
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

void JSONWrap::addClientStats(Client& client) {
	addPair("user_id", client.id);
	addPair("maxcombo", client.stats.maxCombo);
	addPair("maxbpm", client.stats.maxBpm);
    addPair("gamesplayed", client.stats.gamesPlayed);
    addPair("avgbpm", client.stats.avgBpm);
    addPair("gameswon", client.stats.gamesWon);
    addPair("rank", client.stats.rank);
    addPair("points", client.stats.points+1000);
    addPair("heropoints", client.stats.heropoints);
    addPair("totalbpm", client.stats.totalBpm);
    addPair("totalgames", client.stats.totalGames);
    addPair("1vs1points", client.stats.vspoints);
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
	bool end=false;
	jsonString.erase(jsonString.begin());
	jsonString.erase(jsonString.end()-1);
	while (!end) {
		stop = jsonString.find(":", start);
		std::string key = jsonString.substr(start+1, stop-start-2);
		if (stop == std::string::npos)
			break;
		start = stop+1;
		stop = jsonString.find(",", start);
		sf::Uint32 value = stol(jsonString.substr(start, stop-start));
		start = stop+1;
		if (key == "maxcombo") stats.maxCombo = value;
		else if (key == "maxbpm") stats.maxBpm = value;
		else if (key == "rank") stats.rank = value;
		else if (key == "points") stats.points = value-1000;
		else if (key == "heropoints") stats.heropoints = value;
		else if (key == "herorank") stats.herorank = value;
		else if (key == "1vs1points") stats.vspoints = value;
		else if (key == "1vs1rank") stats.vsrank = value;
		else if (key == "avgbpm") stats.avgBpm = value;
		else if (key == "gamesplayed") stats.gamesPlayed = value;
		else if (key == "gameswon") stats.gamesWon = value;
		else if (key == "totalgames") stats.totalGames = value;
		else if (key == "totalbpm") stats.totalBpm = value;
		else if (key == "tournamentsplayed") stats.tournamentsplayed = value;
		else if (key == "tournamentswon") stats.tournamentswon = value;
		else if (key == "gradeA") stats.gradeA = value;
		else if (key == "gradeB") stats.gradeB = value;
		else if (key == "gradeC") stats.gradeC = value;
		else if (key == "gradeD") stats.gradeD = value;
		if (stop == std::string::npos)
			break;
	}
}

sf::Http::Response JSONWrap::sendPost(const sf::String& _request) {
	sf::Http::Request request(_request, sf::Http::Request::Post);
	request.setBody(getJsonString());
    request.setField("Content-Type", "application/x-www-form-urlencoded");
	sf::Http http("http://localhost");
    return http.sendRequest(request);
}

void JSONWrap::clear() {
	pairs.clear();
}