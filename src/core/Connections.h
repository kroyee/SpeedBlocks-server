#ifndef CONNECTTIONS_H
#define CONNECTTIONS_H
#include <SFML/Network.hpp>
#include <list>
#include <thread>
#include <iostream>
#include "PacketCompress.h"
#include "Lobby.h"

class Connections {
public:
	Connections();
	
	std::list<Client> clients;

	std::list<Client> uploadData;

	unsigned short tcpPort;

	uint8_t id;
	sf::TcpListener listener;
	sf::SocketSelector selector;
	sf::UdpSocket udpSock;
	sf::Socket::Status status;
	sf::String serverkey, challongekey;

	sf::Clock serverClock;

	Client* sender;

	sf::IpAddress udpAdd;
	unsigned short udpPort;

	uint16_t clientVersion;

	uint16_t clientCount;

	Lobby lobby;

	PacketCompress extractor;

	bool setUpListener();
	void listen();
	void receive();
	void disconnectClient(Client&);
	void send(Client&);
	void send(Client&, Client&);
	void send(Room&);
	void send(Room&, short);
	void sendUDP(Client& client, sf::Packet& packet);
	void sendSignal(uint8_t signalId, int id1 = -1, int id2 = -1);

	void sendWelcomeMsg();
	void sendAuthResult(uint8_t authresult, Client& client);
	void sendChatMsg(sf::Packet& packet);
	void sendClientJoinedServerInfo(Client& client);
	void sendClientLeftServerInfo(Client& client);

	void validateClient(sf::Packet& packet);
	void validateUDP(sf::Packet& packet);
	void getGamestate(sf::Packet& packet);

	void manageRooms();
	void manageClients();
	void manageUploadData();
	void manageTournaments();
	void manageMatchmaking();

	void handlePacket(sf::Packet& packet);

	bool getKey();
};

#endif