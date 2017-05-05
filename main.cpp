#include <iostream>
#include <SFML/Network.hpp>
#include <thread>
#include "Connections.h"
#include "Tournament.h"
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
	/*//Test
	sf::Http::Request request("/speedblocks/testing.php", sf::Http::Request::Post);

    sf::String stream = "id=1&maxbpm=30&maxcombo=57&avgbpm=65.6785432987";
    request.setBody(stream);

    sf::Http http("http://speedblocks.esy.es");
    sf::Http::Response response = http.sendRequest(request);

    if (response.getStatus() == sf::Http::Response::Ok) {
    	std::cout << response.getBody() << std::endl;
    }
    else
        std::cout << "request failed" << std::endl;
    //TEST END*/

	srand(time(NULL));

	Connections conn;
	if (!conn.setUpListener())
		return 1;

	std::thread t(&getInput);

	std::cout << "Listener set up" << std::endl;

	conn.lobby.addRoom("Standard", 0, 1, 3);
	conn.lobby.idcount=1;
	conn.lobby.addRoom("Fast and Furious", 5, 3, 3);

	conn.lobby.setMsg("Welcome to the server you wonderful beast");

	Tournament tourney;
	for (int i=0; i<5; i++)
		tourney.addPlayer(to_string(i+1) + "Dude", i);

	tourney.makeBracket();
	tourney.putPlayersInBracket();
	tourney.printBracket();
	tourney.collapseBracket();
	tourney.printBracket();

	while (!quit) {
		if (conn.listen())
			if (conn.receive())
				conn.handlePacket();

		conn.manageRooms();
		conn.manageClients();
		if (status) {
			for (auto&& client : conn.clients) {
				std::cout << client.id << ": " << client.name.toAnsiString();
				if (client.room != nullptr)
					std::cout << " in room " << client.room->name.toAnsiString();
				std::cout << std::endl;
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