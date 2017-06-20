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
	Connections() : lobby(this) { clientVersion=4; clientCount=0; idcount=60000; tcpPort=21512; udpSock.bind(21514); selector.add(udpSock); }
	
	std::list<Client> clients;

	std::list<Client> uploadData;

	unsigned short tcpPort;

	sf::Packet packet;
	sf::Uint8 id;
	sf::TcpListener listener;
	sf::SocketSelector selector;
	sf::UdpSocket udpSock;
	sf::Socket::Status status;
	sf::String serverkey;

	sf::Clock serverClock;

	Client* sender;

	sf::IpAddress udpAdd;
	unsigned short udpPort;

	sf::Uint16 idcount, clientVersion;

	sf::Uint16 clientCount;

	Lobby lobby;

	PacketCompress extractor;

	bool setUpListener();
	bool listen();
	bool receive();
	void disconnectClient(Client&);
	void send(Client&);
	void send(Client&, Client&);
	void send(Room&);
	void send(Room&, short);
	void sendUDP(Client& client);
	void sendSignal(sf::Uint8 signalId, int id1 = -1, int id2 = -1);

	void sendWelcomeMsg();
	void sendAuthResult(sf::Uint8 authresult, Client& client);
	void sendChatMsg();
	void sendClientJoinedServerInfo(Client& client);
	void sendClientLeftServerInfo(Client& client);

	void validateClient();
	void validateUDP();
	void getGamestate();

	void manageRooms();
	void manageClients();
	void manageUploadData();
	void manageTournaments();

	void handlePacket();
	void handleSignal();

	bool getKey();
};

#endif