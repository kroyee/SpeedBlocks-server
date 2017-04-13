#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Network.hpp>
#include <thread>
#include <list>
#include <iostream>
#include "MingwConvert.h"

class Room;
class Connections;

class PlayfieldHistory {
public:
	sf::Uint8 square[22][10];
};

class Client {
public:
	Client(Connections* _conn) : conn(_conn), guest(false), away(false) {}
	Client(const Client& client);
	sf::IpAddress address;
	sf::TcpSocket socket;
	sf::Uint16 id;
	sf::Uint8 datacount;
	sf::Packet data;
	sf::Uint16 udpPort;
	Room* room;
	Connections* conn;

	sf::String name;
	sf::String authpass;
	sf::Uint8 authresult;

	bool alive, datavalid, sdataSet, guest, sdataSetFailed, sdataInit, sdataPut, away;

	std::thread thread;

	sf::Uint8 maxCombo, position, garbageCleared;
	sf::Uint16 linesSent, linesReceived, linesBlocked, bpm, spm, score;
	float incLines, linesAdjusted;

	sf::Uint8 s_maxCombo, s_maxBpm, s_rank;
	sf::Int16 s_points, s_heropoints, s_herorank;
	float s_avgBpm;
	sf::Uint32 s_gamesPlayed, s_gamesWon, s_totalGames, s_totalBpm;

	sf::Time uploadTime;

	std::list<PlayfieldHistory> history;

	void sendData();
	void getData();
	int getDataInt(short, short, std::string&);
	float getDataFloat(short, short, std::string&);
	void authUser();
	void copy(Client&);
	void checkIfStatsSet();
	void checkIfAuth();
	void sendLines();
};

#endif