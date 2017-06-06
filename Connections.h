#ifndef CONNECTTIONS_H
#define CONNECTTIONS_H
#include <SFML/Network.hpp>
#include <list>
#include <thread>
#include <iostream>
#include "PacketCompress.h"
#include "Lobby.h"
#include "Tournament.h"

class Connections {
public:
	Connections() : lobby(this) { clientVersion=3; clientCount=0; idcount=60000; tcpPort=21512; udpSock.bind(21514); selector.add(udpSock); }
	
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
	void sendSignal(Client& client, sf::Uint8 signalId, sf::Uint16 id1 = 0, sf::Uint16 id2 = 0);

	void manageRooms();
	void manageClients();
	void manageUploadData();
	void manageTournaments();

	void handlePacket();
	void handleSignal();

	bool getKey();

	//Send packet functions
	void sendPacket0();
	void sendPacket1(Room& room, sf::Uint8 countdown);
	void sendPacket2(Room& room, sf::Uint8 countdown);
	void sendPacket3(Room& it, sf::Uint8 joinok);
	void sendPacket4(Room& room);
	void sendPacket5(Room& room, sf::Uint16 id);
	void sendPacket6(Room& room);
	void sendPacket7(Room& room, Client* winner);
	void sendPacket8();
	void sendPacket9(sf::Uint8 authresult, Client& client);
	void sendPacket10(Client& client, sf::Uint8 amount);
	void sendPacket11(Room& room);
	void sendPacket12(sf::Uint8 type, const sf::String& to, const sf::String& msg);
	void sendPacket13();
	void sendPacket14();
	void sendPacket15(Client& client);
	void sendPacket16(Client& client);
	void sendPacket17(Room& room);
	void sendPacket18(sf::Uint16 id);
	void sendPacket19(Client& client);
	void sendPacket20(Client& client);
	void sendPacket21(Client& client);
	void sendPacket22(Client& client);
	void sendPacket23(Tournament& tournament);
	void sendPacket24(Node& game);
	void sendPacket25();
	void sendPacket26();
	void sendPacket102();
};

#endif