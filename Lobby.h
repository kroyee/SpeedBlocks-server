#ifndef LOBBY_H
#define LOBBY_H

#include <SFML/Network.hpp>
#include "Room.h"
#include "Tournament.h"
#include "Challenges.h"
#include "VSMatch.h"

class Connections;

class Lobby {
public:
	Lobby(Connections* _conn);
	Connections* conn;
	sf::String welcomeMsg;

	std::list<Room> rooms;
	std::list<Room> tmp_rooms;
	std::list<Tournament> tournaments;
	sf::Uint8 roomCount, tournamentCount;
	sf::Uint16 idcount, tourn_idcount, tmp_idcount;

	Tournament *daily, *weekly, *monthly, *grandslam;

	ChallengeHolder challengeHolder;
	VSMatch matchmaking1vs1;

	sf::Time saveTournamentsTime;
	bool tournamentsUpdated;

	void joinRequest();
	void joinRoom(sf::Uint16 roomid);
	void joinTournament(sf::Uint16 tournamentid);
	void joinTournamentGame();
	void joinAsSpectator();
	bool alreadyInside(const Room&, const Client&);

	void addRoom(const sf::String& name, short, sf::Uint16 mode, sf::Uint8 delay);
	void addTempRoom(sf::Uint16 mode, Node* game = nullptr, Tournament* _tournament = nullptr);
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
	void regularTournaments();
	void saveTournaments();
	void loadTournaments();

	void playChallenge();

	void getReplay();

	void pairMatchmaking();
};

#endif