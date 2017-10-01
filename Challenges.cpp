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

ChallengeHolder::ChallengeHolder(Connections& _conn) : conn(_conn), challengeCount(0) {
	loadChallenges();
}

void ChallengeHolder::saveChallenges() {
	if (conn.serverClock.getElapsedTime() < updateTime)
		return;
	updateTime = conn.serverClock.getElapsedTime() + sf::seconds(60);
	for (auto&& chall : challenges) {
		if (!chall->update)
			continue;
		cout << "Saving challenge " << chall->name.toAnsiString() << endl;
		std::ofstream file("Challenges/" + chall->name);
		if (!file.is_open()) {
			cout << "Failed to save challenge " << chall->name.toAnsiString() << endl;
			continue;
		}
		for (auto&& score : chall->scores) {
			file << score.id << endl;
			file << score.name.toAnsiString() << endl;
			file << score.duration << endl;
			for (auto& col : score.column)
				file << col.toAnsiString() << endl;
			if (score.replay.id && score.update)
				score.replay.save("Challenges/" + chall->name + "." + score.name);
			score.update=false;
		}
		chall->update=false;
		file.close();
	}
}

void ChallengeHolder::loadChallenges() {
	challenges.push_back(std::unique_ptr<Challenge>(new CH_Race));
	challenges.push_back(std::unique_ptr<Challenge>(new CH_Cheese));
	challenges.push_back(std::unique_ptr<Challenge>(new CH_Survivor));

	challengeCount = challenges.size();

	updateTime = conn.serverClock.getElapsedTime() + sf::seconds(60);
}

void ChallengeHolder::sendChallengeList(Client& client) {
	conn.packet.clear();
	conn.packet << (sf::Uint8)2 << challengeCount;
	for (auto&& chall : challenges)
		conn.packet << chall->id << chall->name << chall->label;
	conn.send(client);
}

void ChallengeHolder::sendLeaderboard(sf::Uint16 id) {
	for (auto&& chall : challenges)
		if (chall->id == id) {
			conn.packet.clear();
			sf::Uint8 size = chall->columns.size();
			conn.packet << (sf::Uint8)5 << chall->id << size;
			for (auto& col : chall->columns)
				conn.packet << col.width << col.text;

			conn.packet << chall->scoreCount;
			for (auto&& score : chall->scores) {
				conn.packet << score.name;
				for (auto& col : score.column)
					conn.packet << col;
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
		if (chall->id == client.room->gamemode) {
			for (auto&& score : chall->scores)
				if (score.id == client.id) {
					if (chall->checkResult(client, duration, blocks, score)) {
						sendReplayRequest(client, durationToString(score.duration) + " to " + durationToString(duration));
						score.duration = duration;
						score.replay.id = 0;
						score.update=true;
						chall->update=true;
						chall->scores.sort([&](Score& s1, Score& s2){ return chall->sort(s1, s2); });
					}
					else
						sendNotGoodEnough(client, durationToString(duration) + " did not beat " + durationToString(score.duration));
					return;
				}
			sendReplayRequest(client, "nothing to " + durationToString(duration));
			chall->addScore(client, duration, blocks);
			chall->scores.sort([&](Score& s1, Score& s2){ return chall->sort(s1, s2); });
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
		if (chall->id == id) {
			for (auto&& score : chall->scores)
				if (score.id == client.id) {
					score.replay.receiveRecording(conn.packet);
					score.replay.id = client.id;
					score.update=true;
					chall->update=true;
					if (client.room != nullptr)
						client.room->waitForReplay=false;
					sendLeaderboard(id);
					score.replay.packet << (sf::Uint8)200 << client.name;
					return;
				}
			return;
		}
}

void ChallengeHolder::sendReplay() {
	sf::Uint16 id, slot;
	conn.packet >> id >> slot;
	for (auto&& chall : challenges)
		if (chall->id == id) {
			if (slot >= chall->scores.size())
				return;
			auto it = chall->scores.begin();
			for (int i=0; i<slot; i++)
				it++;
			it->replay.sendRecording(*conn.sender);
		}
}

//////////////////////////////////////////////////////////////////////////
//								Base Challenge 							//
//////////////////////////////////////////////////////////////////////////

bool Challenge::sort(Score& score1, Score& score2) {
	return score1.duration < score2.duration;
}

void Challenge::addScore(Client& client, sf::Uint32 duration, sf::Uint16 blocks) {
	Score score;
	score.id = client.id;
	score.name = client.name;
	score.duration = duration;
	score.column.resize(columns.size()-1);
	this->setColumns(client, blocks, score);
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

	while (getline(file, line)) {
		if (line == "")
			continue;
		score.id = stoi(line);
		getline(file, line);
		score.name = line;
		getline(file, line);
		score.duration = stoi(line);
		int size = columns.size()-1;
		score.column.resize(size);
		for (int i=0; i<size; i++) {
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
	scores.sort([&](Score& s1, Score& s2){ return sort(s1, s2); });
}

//////////////////////////////////////////////////////////////////////////
//								Sub Structs								//
//////////////////////////////////////////////////////////////////////////

//////////////////// Race ////////////////////////

CH_Race::CH_Race() {
	name = "Race";
	id = 20000;
	label = "Clear 40 lines";
	columns.push_back(Column(0, "Name"));
	columns.push_back(Column(200, "Blocks"));
	columns.push_back(Column(270, "Time"));
	loadScores();
}

void CH_Race::setColumns(Client&, sf::Uint16 blocks, Score& score) {
	score.column[0] = to_string(blocks);
	score.column[1] = durationToString(score.duration);
}

bool CH_Race::checkResult(Client&, sf::Uint32 duration, sf::Uint16 blocks, Score& score) {
	if (duration < score.duration) {
		score.column[0] = to_string(blocks);
		score.column[1] = durationToString(duration);
		return true;
	}
	return false;
}

/////////////////// Cheese //////////////////////

CH_Cheese::CH_Cheese() {
	name = "Cheese";
	id = 20001;
	label = "Clear all the garbage";
	columns.push_back(Column(0, "Name"));
	columns.push_back(Column(200, "Blocks"));
	columns.push_back(Column(270, "Time"));
	loadScores();
}

void CH_Cheese::setColumns(Client&, sf::Uint16 blocks, Score& score) {
	score.column[0] = to_string(blocks);
	score.column[1] = durationToString(score.duration);
}

bool CH_Cheese::checkResult(Client&, sf::Uint32 duration, sf::Uint16 blocks, Score& score) {
	if (duration < score.duration) {
		score.column[0] = to_string(blocks);
		score.column[1] = durationToString(duration);
		return true;
	}
	return false;
}

/////////////////// Survivor //////////////////////

CH_Survivor::CH_Survivor() {
	name = "Survivor";
	id = 20002;
	label = "Survive as long as possible";
	columns.push_back(Column(0, "Name"));
	columns.push_back(Column(200, "Cleared"));
	columns.push_back(Column(270, "Time"));
	loadScores();
}

void CH_Survivor::setColumns(Client& client, sf::Uint16, Score& score) {
	score.column[0] = to_string(client.garbageCleared);
	score.column[1] = durationToString(score.duration);
}

bool CH_Survivor::checkResult(Client& client, sf::Uint32 duration, sf::Uint16, Score& score) {
	if (duration > score.duration) {
		score.column[0] = to_string(client.garbageCleared);
		score.column[1] = durationToString(duration);
		return true;
	}
	return false;
}

bool CH_Survivor::sort(Score& score1, Score& score2) {
	return score1.duration > score2.duration;
}