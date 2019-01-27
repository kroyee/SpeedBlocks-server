#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Network.hpp>
#include <thread>
#include <list>
#include <iostream>
#include <atomic>
#include "ClientHistory.h"
#include "PingHandle.h"
#include "StatsHolders.h"
#include "Handicap.h"

class Room;
class Connections;
class Tournament;
class JSONWrap;

class Client {
public:
	uint16_t id;
	uint8_t datacount;
	sf::Packet data; // Holds current gamestate

	Room *room = nullptr, *spectating = nullptr;

	std::string name;

	std::atomic<bool> alive{false}, datavalid{false};
	bool sdataSet=false, guest=false, sdataSetFailed=false;
	bool matchmaking=false, sdataPutFailed=false, sdataInit=false, sdataPut=false, away=false, ready=false;

	RoundStats roundStats;
	StatsHolder stats;

	Handicap hcp;
	PlayfieldHistory history;

	Tournament* tournament = nullptr;

	void sendData();
	bool sendDataPart(JSONWrap& jwrap);
	void getData();
	void checkIfStatsSet();
	void goAway();
	void unAway();

	void clear();

	virtual void getRoundData(sf::Packet& packet);
	void getWinnerData(sf::Packet& packet);
	void makeWinner();
	void sendPositionBpm();

	void sendJoinRoomResponse(Room& room, uint16_t joinok);
	void sendAlert(const std::string& msg);

	virtual bool initHuman(sf::TcpListener&, sf::SocketSelector&, const sf::Time&) { return false; }
	virtual void disconnect() {}

	virtual void sendPacket(sf::Packet&) {}
	virtual void sendSignal(uint8_t, int = -1, int = -1) {}
	virtual void sendGameData(sf::UdpSocket&) {}
	virtual void sendGameDataOut(sf::UdpSocket&, sf::Packet&) {}
	virtual void countDown(const sf::Time&) {}
	virtual void sendLines();
	virtual uint16_t sendLinesOut() { return 0; }
	virtual void seed(uint16_t, uint16_t, uint8_t = 0) {}
	virtual void endRound() {}
	virtual void startGame() {}
	virtual bool isHuman() { return false; }
	virtual void updateSpeed() {}

	virtual ~Client() = default;
};

class HumanClient : public Client {
public:
	sf::IpAddress address;
	sf::TcpSocket socket;
	uint16_t udpPort;
	uint8_t authresult = 0;
	sf::Time uploadTime, updateStatsTime{sf::seconds(0)};
	sf::Time lastHeardFrom;
	PingHandle ping;

	void authUser();
	void checkIfAuth();
	void sendPacket(sf::Packet&) override;
	void sendSignal(uint8_t signalId, int id1 = -1, int id2 = -1) override;
	void sendGameData(sf::UdpSocket&) override;
	void sendGameDataOut(sf::UdpSocket&, sf::Packet&) override;
	void countDown(const sf::Time& t) override;
	void seed(uint16_t, uint16_t, uint8_t = 0) override;
	void endRound() override;
	void startGame() override;

	bool initHuman(sf::TcpListener&, sf::SocketSelector&, const sf::Time&) override;
	void disconnect() override { socket.disconnect(); }
	bool isHuman() override { return true; }
};

#endif
