#ifndef LOBBY_H
#define LOBBY_H

#include <SFML/Network.hpp>
#include "Room.h"

class Connections;

class Lobby {
public:
	Lobby(Connections* _conn) : conn(_conn), roomCount(0), idcount(0) {}
	Connections* conn;
	sf::String welcomeMsg;

	std::list<Room> rooms;
	sf::Uint8 roomCount;
	sf::Uint16 idcount;

	void addRoom(const sf::String& name, short, sf::Uint8);
	void removeIdleRooms();
	void setMsg(const sf::String& msg);
	void sendRoomList(Client&);
};

#endif