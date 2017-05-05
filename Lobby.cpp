#include "Lobby.h"
#include "Connections.h"
using std::cout;
using std::endl;

void Lobby::sendRoomList(Client& client) {
	conn->sendPacket16(client);
}

void Lobby::addRoom(const sf::String& name, short max, sf::Uint8 mode, sf::Uint8 delay) {
	Room newroom(conn);
	rooms.push_back(newroom);
	rooms.back().name = name;
	rooms.back().id = idcount;
	rooms.back().maxPlayers = max;
	rooms.back().currentPlayers = 0;
	rooms.back().activePlayers = 0;
	rooms.back().countdownSetting = 3;
	rooms.back().gamemode = mode;
	rooms.back().timeBetweenRounds = sf::seconds(delay);
	cout << "Adding room " << rooms.back().name.toAnsiString() << " as " << rooms.back().id << endl;
	idcount++;
	roomCount++;
	if (idcount<10)
		idcount=10;

	conn->sendPacket17(rooms.back());
}

void Lobby::removeIdleRooms() {
	for (auto it = rooms.begin(); it != rooms.end(); it++)
		if (it->currentPlayers == 0 && it->start.getElapsedTime() > sf::seconds(60) && it->id > 9) {
			cout << "Removing room " << it->name.toAnsiString() << " as " << it->id << endl;
			conn->sendPacket18(it->id);
			it = rooms.erase(it);
			roomCount--;
		}
}

void Lobby::setMsg(const sf::String& msg) {
	welcomeMsg = msg;
}