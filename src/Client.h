#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Network.hpp>
#include <thread>
#include <list>
#include <iostream>
#include "ClientHistory.h"
#include "PingHandle.h"

class Room;
class Connections;
class Tournament;
class JSONWrap;

class StatsHolder {
public:
	// General
	sf::Uint8 maxCombo=0, maxBpm=0, alert=1;
	float avgBpm=0;
	sf::Uint32 totalPlayed=0, totalWon=0, totalBpm=0, challenges_played=0;

	// 1v1
	sf::Uint16 vsPoints=1500, vsRank=0;
	sf::Uint32 vsPlayed=0, vsWon=0;

	// FFA
	sf::Int16 ffaPoints=0;
	sf::Uint8 ffaRank=25;
	sf::Uint32 ffaPlayed=0, ffaWon=0;

	// Hero
	sf::Uint16 heroPoints=1500, heroRank=0;
	sf::Uint32 heroPlayed=0, heroWon=0;

	// Tournament
	sf::Uint16 gradeA=0, gradeB=0, gradeC=0, gradeD=0;
	sf::Uint32 tournamentsPlayed=0, tournamentsWon=0;
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
	bool sendDataPart(JSONWrap& jwrap);
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