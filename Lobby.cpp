#include "Lobby.h"
#include "Connections.h"
using std::cout;
using std::endl;

void Lobby::sendRoomList(Client& client) {
	conn->packet.clear();
	sf::Uint8 packetid = 16; //16-Packet
	conn->packet << packetid << roomCount;
	for (auto&& room : rooms) {
		conn->packet << room.id << room.name << room.currentPlayers << room.maxPlayers;
	}
	conn->send(client);
}

void Lobby::addRoom(const sf::String& name, short max, sf::Uint8 mode) {
	Room newroom(conn);
	rooms.push_back(newroom);
	rooms.back().name = name;
	rooms.back().id = idcount;
	rooms.back().maxPlayers = max;
	rooms.back().currentPlayers = 0;
	rooms.back().activePlayers = 0;
	rooms.back().countdownSetting = 3;
	rooms.back().gamemode = mode;
	cout << "Adding room " << rooms.back().name.toAnsiString() << " as " << rooms.back().id << endl;
	idcount++;
	roomCount++;
	if (idcount<10)
		idcount=10;

	sf::Uint8 packetid = 17; //17-Packet
	conn->packet.clear();
	conn->packet << packetid << rooms.back().id << name << 1 << rooms.back().maxPlayers;
	for (auto&& client : conn->clients)
		conn->send(client);
}

void Lobby::removeIdleRooms() {
	sf::Uint16 id;
	for (auto it = rooms.begin(); it != rooms.end(); it++)
		if (it->currentPlayers == 0 && it->start.getElapsedTime() > sf::seconds(60) && it->id > 9) {
			cout << "Removing room " << it->name.toAnsiString() << " as " << it->id << endl;
			id = it->id;
			it = rooms.erase(it);
			roomCount--;
			sf::Uint8 packetid = 18; //18-Packet
			conn->packet.clear();
			conn->packet << packetid << id;
			for (auto&& client : conn->clients)
				conn->send(client);
		}
}

void Lobby::setMsg(const sf::String& msg) {
	welcomeMsg = msg;
}