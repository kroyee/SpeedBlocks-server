#include <iostream>
#include <SFML/Network.hpp>
#include <thread>
#include "Connections.h"
using std::cout;
using std::endl;

bool quit=false;
bool status=false;

void getInput() {
	std::string s;
	while (!quit) {
		std::cin >> s;
		if (s == "quit")
			quit=true;
		else if (s == "status")
			status=true;
	}
}

int main() {
	srand(time(NULL));

	Connections conn;

	if (!conn.getKey()) {
		cout << "Failed to get serverkey" << endl;
		return 1;
	}

	if (!conn.setUpListener())
		return 1;

	std::thread t(&getInput);

	cout << "Listener set up" << endl;

	conn.lobby.idcount=1;
	conn.lobby.addRoom("Standard", 0, 1, 3);
	conn.lobby.idcount=2;
	conn.lobby.addRoom("Fast and Furious", 5, 3, 3);
	conn.lobby.idcount=10;

	conn.lobby.setMsg("Welcome to the server you wonderful beast");

	conn.lobby.addTournament("Daily one", 0);
	conn.lobby.tournaments.back().setStartingTime(0, 19, 30);
	conn.lobby.daily = &conn.lobby.tournaments.back();

	while (!quit) {
		if (conn.listen())
			if (conn.receive())
				conn.handlePacket();

		conn.manageRooms();
		conn.manageClients();
		conn.manageTournaments();
		if (status) {
			for (auto&& client : conn.clients) {
				cout << client.id << ": " << client.name.toAnsiString();
				if (client.room != nullptr)
					cout << " in room " << client.room->name.toAnsiString();
				cout << endl;
			}
			status=false;
		}
	}

	for (auto&& it : conn.clients) {
		it.socket.disconnect();
	}
	conn.listener.close();
	conn.udpSock.unbind();

	t.join();
	return 0;
}