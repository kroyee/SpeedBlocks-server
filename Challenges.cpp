#include "Challenges.h"
#include "Connections.h"
#include "Client.h"
#include <fstream>
#include <string>
using std::cout;
using std::endl;

sf::String durationToString(sf::Uint32 duration) {
	sf::Uint8 minutes=0, seconds=0, parts=0;
	while (duration >= 60000) {
		minutes++;
		duration-=60000;
	}
	while (duration >= 1000) {
		seconds++;
		duration-=1000;
	}
	while (duration >= 10) {
		parts++;
		duration-=10;
	}
	sf::String asString = to_string(minutes) + ":";
	if (seconds < 10)
		asString += "0";
	asString += to_string(seconds) + ":";
	if (parts < 10)
		asString += "0";
	asString += to_string(parts);

	return asString;
}

bool sortScore(Score& score1, Score& score2) {
	return score1.duration < score2.duration;
}

void Challenge::addScore(Client& client, sf::Uint32 duration, sf::Uint16 blocks) {
	Score score;
	score.id = client.id;
	score.name = client.name;
	score.column[0] = to_string(blocks);
	score.column[1] = durationToString(duration);
	score.duration = duration;
	score.replay.id = 0;
	score.update=true;
	update=true;
	scores.push_back(score);
	scoreCount++;
}

void Challenge::loadScores() {
	std::ifstream file("Challenges/" + name);

	if (!file.is_open()) {
		cout << "Failed to load scores for " << name.toAnsiString() << endl;
		return;
	}

	cout << "Loading scores for " << name.toAnsiString() << endl;

	Score score;
	std::string line;
	scoreCount=0;

	while (!file.eof()) {
		getline(file, line);
		if (line == "")
			continue;
		score.id = stoi(line);
		getline(file, line);
		score.name = line;
		getline(file, line);
		score.duration = stoi(line);
		for (int i=0; i<columns-1; i++) {
			getline(file, line);
			score.column[i] = line;
		}
		score.update=false;
		if (score.replay.load("Challenges/" + name + "." + score.name))
			score.replay.id = score.id;
		else
			score.replay.id = 0;
		scores.push_back(score);
		scoreCount++;
	}
	file.close();
	scores.sort(&sortScore);
}

ChallengeHolder::ChallengeHolder(Connections& _conn) : conn(_conn), challengeCount(0) {
	loadChallenges();
}

void ChallengeHolder::saveChallenges() {
	if (conn.serverClock.getElapsedTime() < updateTime)
		return;
	updateTime = conn.serverClock.getElapsedTime() + sf::seconds(60);
	for (auto&& chall : challenges) {
		if (!chall.update)
			continue;
		cout << "Saving challenge " << chall.name.toAnsiString() << endl;
		std::ofstream file("Challenges/" + chall.name);
		if (!file.is_open()) {
			cout << "Failed to save challenge " << chall.name.toAnsiString() << endl;
			continue;
		}
		for (auto&& score : chall.scores) {
			file << score.id << endl;
			file << score.name.toAnsiString() << endl;
			file << score.duration << endl;
			for (int i=0; i<chall.columns-1; i++)
				file << score.column[i].toAnsiString() << endl;
			if (score.replay.id && score.update)
				score.replay.save("Challenges/" + chall.name + "." + score.name);
			score.update=false;
		}
		chall.update=false;
	}
}

void ChallengeHolder::loadChallenges() {
	std::ifstream file("Challenges/challenges.list");

	if (!file.is_open()) {
		cout << "Failed to load challenges" << endl;
		return;
	}

	cout << "Loading challenges" << endl;

	std::string line;
	Challenge chall;

	while (!file.eof()) {
		getline(file, line);
		if (line == "")
			continue;
		chall.name = line;
		getline(file, line);
		chall.id = stoi(line);
		getline(file, line);
		chall.label = line;
		getline(file, line);
		chall.columns = stoi(line);
		for (int i=0; i<chall.columns; i++) {
			getline(file, line);
			chall.column[i] = line;
			getline(file, line);
			chall.width[i] = stoi(line);
		}
		challenges.push_back(chall);
		challengeCount++;
		challenges.back().loadScores();
	}

	updateTime = conn.serverClock.getElapsedTime() + sf::seconds(60);
}

void ChallengeHolder::sendChallengeList(Client& client) {
	conn.packet.clear();
	conn.packet << (sf::Uint8)2 << challengeCount;
	for (auto&& chall : challenges)
		conn.packet << chall.id << chall.name << chall.label;
	conn.send(client);
}

void ChallengeHolder::sendLeaderboard(sf::Uint16 id) {
	for (auto&& chall : challenges)
		if (chall.id == id) {
			conn.packet.clear();
			conn.packet << (sf::Uint8)5 << chall.id << chall.columns;
			for (int i=0; i<chall.columns; i++)
				conn.packet << chall.width[i];
			for (int i=0; i<chall.columns; i++)
				conn.packet << chall.column[i];
			conn.packet << chall.scoreCount;
			for (auto&& score : chall.scores) {
				conn.packet << score.name;
				for (int i=0; i<chall.columns-1; i++)
					conn.packet << score.column[i];
			}
			conn.send(*conn.sender);
			return;
		}
}

void ChallengeHolder::checkResult(Client& client) {
	sf::Uint32 duration;
	sf::Uint16 blocks;

	conn.packet >> duration >> blocks;

	for (auto&& chall : challenges)
		if (chall.id == client.room->gamemode) {
			for (auto&& score : chall.scores)
				if (score.id == client.id) {
					if (duration < score.duration) {
						sendReplayRequest(client, durationToString(score.duration) + " to " + durationToString(duration));
						score.duration = duration;
						score.column[0] = to_string(blocks);
						score.column[1] = durationToString(duration);
						score.replay.id = 0;
						score.update=true;
						chall.update=true;
						chall.scores.sort(&sortScore);
					}
					else
						sendNotGoodEnough(client, durationToString(duration) + " did not beat " + durationToString(score.duration));
					return;
				}
			sendReplayRequest(client, "nothing to " + durationToString(duration));
			chall.addScore(client, duration, blocks);
			chall.scores.sort(&sortScore);
			return;
		}
}

void ChallengeHolder::sendReplayRequest(Client& client, sf::String text) {
	conn.packet.clear();
	conn.packet << (sf::Uint8)6 << text;
	conn.send(client);
	if (client.room != nullptr)
		client.room->waitForReplay=true;
}

void ChallengeHolder::sendNotGoodEnough(Client& client, sf::String text) {
	conn.packet.clear();
	conn.packet << (sf::Uint8)7 << text;
	conn.send(client);
}

void ChallengeHolder::updateResult(Client& client, sf::Uint16 id) {
	for (auto&& chall : challenges)
		if (chall.id == id) {
			for (auto&& score : chall.scores)
				if (score.id == client.id) {
					score.replay.receiveRecording(conn.packet);
					score.replay.id = client.id;
					score.update=true;
					chall.update=true;
					if (client.room != nullptr)
						client.room->waitForReplay=false;
					sendLeaderboard(id);
					return;
				}
			return;
		}
}

void ChallengeHolder::sendReplay() {
	sf::Uint16 id, slot;
	conn.packet >> id >> slot;
	for (auto&& chall : challenges)
		if (chall.id == id) {
			if (slot >= chall.scores.size())
				return;
			auto it = chall.scores.begin();
			for (int i=0; i<slot; i++)
				it++;
			it->replay.sendRecording(*conn.sender);
		}
}