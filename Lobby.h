#ifndef LOBBY_H
#define LOBBY_H

#include <SFML/Network.hpp>
#include "Room.h"
#include "Tournament.h"

class Connections;

class Lobby {
public:
	Lobby(Connections* _conn) : conn(_conn), roomCount(0), tournamentCount(0), idcount(0), tourn_idcount(40000), tmp_idcount(50000), daily(nullptr) {}
	Connections* conn;
	sf::String welcomeMsg;

	std::list<Room> rooms;
	std::list<Room> tmp_rooms;
	std::list<Tournament> tournaments;
	sf::Uint8 roomCount, tournamentCount;
	sf::Uint16 idcount, tourn_idcount, tmp_idcount;

	Tournament* daily;

	void joinRequest();
	void joinRoom(sf::Uint16 roomid);
	void joinTournament(sf::Uint16 tournamentid);
	void joinTournamentGame();
	bool alreadyInside(const Room&, const Client&);

	void addRoom(const sf::String& name, short, sf::Uint8 mode, sf::Uint8 delay);
	void addTempRoom(sf::Uint8 mode, Node* game = nullptr, Tournament* _tournament = nullptr);
	void removeIdleRooms();
	void setMsg(const sf::String& msg);
	void sendRoomList(Client&);

	void addTournament(const sf::String& name, sf::Uint16 _mod_id);
	void removeTournament(sf::Uint16 id);
	void sendTournamentList(Client&);
	void signUpForTournament(Client&);
	void withdrawFromTournament(Client&);
	void closeSignUp();
	void startTournament();
	void removeTournamentObserver();
	void createTournament();
	void dailyTournament();
};

#endif