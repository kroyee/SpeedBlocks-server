#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Network.hpp>
#include <thread>
#include <list>
#include <iostream>
#include "ClientHistory.h"
#include "PingHandle.h"
#include "StatsHolders.h"

class Room;
class Connections;
class Tournament;
class JSONWrap;

class Client {
public:
	Client(Connections* _conn);
	sf::IpAddress address;
	sf::TcpSocket *socket;
	uint16_t id;
	uint8_t datacount;
	sf::Packet data;
	uint16_t udpPort;
	Room* room, *spectating;
	Connections* conn;

	sf::String name;
	uint8_t authresult;

	bool alive, datavalid, sdataSet, guest, sdataSetFailed, sdataPutFailed, sdataInit, sdataPut, away, ready;
	bool matchmaking;

	std::thread *thread;

	RoundStats roundStats;

	sf::Time uploadTime, updateStatsTime;
	sf::Time lastHeardFrom;

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

	void getRoundData(sf::Packet& packet);
	void getWinnerData(sf::Packet& packet);

	void sendPacket(sf::Packet&);
	void sendSignal(uint8_t signalId, int id1 = -1, int id2 = -1);
	void sendJoinRoomResponse(Room& room, uint16_t joinok);
	void sendAlert(const sf::String& msg);
};

#endif