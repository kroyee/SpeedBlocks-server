#ifndef NETWORK_H
#define NETWORK_H
#include <SFML/Network.hpp>
#include <list>
#include <thread>
#include <iostream>
#include "PacketCompress.h"

class Room;
class Connections;
class Lobby;

class Adjust {
public:
	short players;
	short amount;
};

class PlayfieldHistory {
public:
	sf::Uint8 square[22][10];
};

class Client {
public:
	Client() { guest=true; away=false; }
	Client(const Client& client);
	sf::IpAddress address;
	sf::TcpSocket socket;
	sf::Uint16 id;
	sf::Uint8 datacount;
	sf::Packet data;
	sf::Uint16 udpPort;
	Room* room;

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
};

class Room {
public:
	Room(Connections* _conn) : active(false), round(false), conn(_conn), hero(false) {}
	Connections* conn;
	sf::String name;
	sf::Uint16 id;

	sf::Uint8 maxPlayers;
	sf::Uint8 currentPlayers;
	sf::Uint8 activePlayers;

	std::list<Client*> clients;

	std::list<Client> leavers;

	std::list<Adjust> adjust;

	//Room setting
	sf::Clock start;
	bool active;
	short countdownSetting;
	short countdown;
	bool round;
	sf::Time countdownTime, roundLenght;
	sf::Uint8 playersAlive;

	bool hero;

	void join(Client&);
	void leave(Client&);
	void startGame();
	void endRound();
	void scoreRound();
	void transfearScore();
	void updatePlayerScore();
	void playerDied();
};

class Lobby {
public:
	Lobby(Connections* _conn) : conn(_conn), idcount(0), roomCount(0) {}
	Connections* conn;
	sf::String welcomeMsg;

	std::list<Room> rooms;
	sf::Uint8 roomCount;
	sf::Uint16 idcount;

	void addRoom(const sf::String& name, short);
	void removeRoom(sf::Uint16 id);
	void setMsg(const sf::String& msg);
};

class Connections {
public:
	Connections() : lobby(this) { clientVersion=1; clientCount=0; idcount=60000; tcpPort=21512; udpPort=21513; udpSock.bind(21514); selector.add(udpSock); }
	
	std::list<Client> clients;

	std::list<Client> uploadData;

	unsigned short tcpPort;
	unsigned short udpPort;

	sf::Packet packet;
	sf::Uint8 id;
	bool gameData;
	sf::TcpListener listener;
	sf::SocketSelector selector;
	sf::UdpSocket udpSock;
	sf::Socket::Status status;

	sf::Clock uploadClock;

	Client* sender;

	sf::IpAddress udpAdd;
	unsigned short udpPortRec;

	sf::Uint16 idcount, clientVersion;

	sf::Uint8 clientCount;

	Lobby lobby;

	PacketCompress extractor;

	bool setUpListener();
	bool listen();
	bool receive();
	void send(Client&);
	void send(Client&, Client&);
	void send(Room&);
	void send(Room&, short);

	void sendWelcome();
	void manageRooms();
	void manageClients();

	void handle();
};

#endif