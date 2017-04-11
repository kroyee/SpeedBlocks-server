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