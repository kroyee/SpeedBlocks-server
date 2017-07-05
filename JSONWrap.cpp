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
	jwrap.addPair("user_id", client.id);
	jwrap.addPair("maxcombo", client.s_maxCombo);
	jwrap.addPair("maxbpm", client.s_maxBpm);
    jwrap.addPair("gamesplayed", client.s_gamesPlayed);
    jwrap.addPair("avgbpm", client.s_avgBpm);
    jwrap.addPair("gameswon", client.s_gamesWon);
    jwrap.addPair("rank", client.s_rank);
    jwrap.addPair("points", client.s_points+1000);
    jwrap.addPair("heropoints", client.s_heropoints);
    jwrap.addPair("totalbpm", client.s_totalBpm);
    jwrap.addPair("totalgames", client.s_totalGames); 
    jwrap.addPair("herorank", client.s_herorank);
    jwrap.addPair("1vs1points", client.s_1vs1points);
    jwrap.addPair("1vs1rank", client.s_1vs1rank);
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

void JSONWrap::jsonToClientStats(Client& client, std::string jsonString) {
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
		sf::Uint32 value = stoi(jsonString.substr(start, stop-start));
		start = stop+1;
		if (key == "maxcombo") client.s_maxCombo = value;
		else if (key == "maxbpm") client.s_maxBpm = value;
		else if (key == "rank") client.s_rank = value;
		else if (key == "points") client.s_points = value-1000;
		else if (key == "heropoints") client.s_heropoints = value;
		else if (key == "herorank") client.s_herorank = value;
		else if (key == "1vs1points") client.s_1vs1points = value;
		else if (key == "1vs1rank") client.s_1vs1rank = value;
		else if (key == "avgbpm") client.s_avgBpm = value;
		else if (key == "gamesplayed") client.s_gamesPlayed = value;
		else if (key == "gameswon") client.s_gamesWon = value;
		else if (key == "totalgames") client.s_totalGames = value;
		else if (key == "totalbpm") client.s_totalBpm = value;
		else if (key == "tournamentsplayed") client.s_tournamentsplayed = value;
		else if (key == "tournamentswon") client.s_tournamentswon = value;
		else if (key == "gradeA") client.s_gradeA = value;
		else if (key == "gradeB") client.s_gradeB = value;
		else if (key == "gradeC") client.s_gradeC = value;
		else if (key == "gradeD") client.s_gradeD = value;
		if (stop == std::string::npos)
			break;
	}
}

sf::Http::Response JSONWrap::sendPost(const sf::String& _request) {
	sf::Http::Request request(_request, sf::Http::Request::Post);
	request.setBody(jwrap.getJsonString());
    request.setField("Content-Type", "application/x-www-form-urlencoded");
	sf::Http http("http://localhost");
    return http.sendRequest(request);
}

void JSONWrap::clear() {
	pairs.clear();
}