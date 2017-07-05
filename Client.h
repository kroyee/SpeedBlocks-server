#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Network.hpp>
#include <thread>
#include <list>
#include <iostream>
#include "MingwConvert.h"
#include "ClientHistory.h"

class Room;
class Connections;
class Tournament;

class Client {
public:
	Client(Connections* _conn) : conn(_conn), guest(false), away(false), history(*this) {}
	Client(const Client& client);
	sf::IpAddress address;
	sf::TcpSocket socket;
	sf::Uint16 id;
	sf::Uint8 datacount;
	sf::Packet data;
	sf::Uint16 udpPort;
	Room* room, *spectating;
	Connections* conn;

	sf::String name;
	sf::Uint8 authresult;

	bool alive, datavalid, sdataSet, guest, sdataSetFailed, sdataPutFailed, sdataInit, sdataPut, away, ready;

	std::thread thread;

	sf::Uint8 maxCombo, position, garbageCleared;
	sf::Uint16 linesSent, linesReceived, linesBlocked, bpm, spm, score;
	float incLines, linesAdjusted;

	sf::Uint8 s_maxCombo, s_maxBpm, s_rank;
	sf::Int16 s_points;
	sf::Uint16 s_heropoints, s_herorank, s_1vs1points, s_1vs1rank, s_gradeA, s_gradeB, s_gradeC, s_gradeD;
	float s_avgBpm;
	sf::Uint32 s_gamesPlayed, s_gamesWon, s_totalGames, s_totalBpm, s_tournamentsplayed, s_tournamentswon;

	sf::Time uploadTime;
	sf::Time lastHeardFrom;

	sf::Uint8 pingId;
	sf::Time pingStart, pingTime;

	PlayfieldHistory history;

	Tournament* tournament;

	void sendData();
	void getData();
	int getDataInt(short, short, std::string&);
	float getDataFloat(short, short, std::string&);
	void authUser();
	void copy(Client&);
	void checkIfStatsSet();
	void checkIfAuth();
	void sendLines();
	void goAway();
	void unAway();

	void getRoundData();
	void getWinnerData();

	void sendSignal(sf::Uint8 signalId, int id1 = -1, int id2 = -1);
};

#endif