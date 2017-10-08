#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Network.hpp>
#include <thread>
#include <list>
#include <iostream>
#include "MingwConvert.h"
#include "ClientHistory.h"
#include "PingHandle.h"

class Room;
class Connections;
class Tournament;

class StatsHolder {
public:
	sf::Uint8 maxCombo, maxBpm, rank, alert;
	sf::Int16 points;
	sf::Uint16 heropoints, herorank, vspoints, vsrank, gradeA, gradeB, gradeC, gradeD;
	float avgBpm;
	sf::Uint32 gamesPlayed, gamesWon, totalGames, totalBpm, tournamentsplayed, tournamentswon, challenges_played;
};

class Client {
public:
	Client(Connections* _conn);
	sf::IpAddress address;
	sf::TcpSocket *socket;
	sf::Uint16 id;
	sf::Uint8 datacount;
	sf::Packet data;
	sf::Uint16 udpPort;
	Room* room, *spectating;
	Connections* conn;

	sf::String name;
	sf::Uint8 authresult;

	bool alive, datavalid, sdataSet, guest, sdataSetFailed, sdataPutFailed, sdataInit, sdataPut, away, ready;
	bool matchmaking;

	std::thread *thread;

	sf::Uint8 maxCombo, position, garbageCleared;
	sf::Uint16 linesSent, linesReceived, linesBlocked, bpm, spm, score;
	float incLines, linesAdjusted;

	sf::Time uploadTime, updateStatsTime;
	sf::Time lastHeardFrom;

	sf::Uint8 pingId;
	sf::Time pingStart, pingTime;

	PingHandle ping;
	StatsHolder stats;

	PlayfieldHistory history;

	Tournament* tournament;

	void sendData();
	void getData();
	int getDataInt(short, short, std::string&);
	float getDataFloat(short, short, std::string&);
	void authUser();
	void checkIfStatsSet();
	void checkIfAuth();
	void sendLines();
	void goAway();
	void unAway();

	void getRoundData();
	void getWinnerData();

	void sendPacket(sf::Packet&);
	void sendSignal(sf::Uint8 signalId, int id1 = -1, int id2 = -1);
	void sendJoinRoomResponse(Room& room, sf::Uint16 joinok);
	void sendAlert(const sf::String& msg);
};

#endif