#ifndef LOBBY_H
#define LOBBY_H

#include <SFML/Network.hpp>
#include "Room.h"
#include "Tournament.h"
#include "Challenges.h"
#include "VSMatch.h"
#include "AIManager.h"
#include <memory>

class Connections;

class Lobby {
public:
	Lobby(Connections& _conn);
	Connections& conn;
	std::string welcomeMsg;

	std::list< std::unique_ptr<Room> > rooms;
	std::list< std::unique_ptr<Room> > tmp_rooms;
	std::list<Tournament> tournaments;
	uint8_t roomCount, tournamentCount;
	uint16_t idcount, tourn_idcount, tmp_idcount;

	Tournament *daily, *weekly, *monthly, *grandslam;

	ChallengeHolder challengeHolder;
	VSMatch matchmaking1vs1;
	AIManager aiManager;

	sf::Time saveTournamentsTime;
	bool tournamentsUpdated;

	void joinRequest(uint16_t id);
	void joinRoom(uint16_t roomid);
	void joinTournament(uint16_t tournamentid);
	void joinTournamentGame(uint16_t tId, uint16_t gId);
	void joinAsSpectator(uint16_t id, uint16_t gId);
	bool alreadyInside(const Room&, const Client&);

	void addRoom(const std::string& name, short, uint16_t mode, uint8_t delay);
	void addTempRoom(uint16_t mode, Node* game = nullptr, Tournament* _tournament = nullptr);
	void removeIdleRooms();
	void setMsg(const std::string& msg);
	void sendRoomList(Client&);

	void addTournament(const std::string& name, uint16_t _mod_id);
	void removeTournament(uint16_t id);
	void sendTournamentList(Client&);
	void signUpForTournament(HumanClient&, uint16_t id);
	void withdrawFromTournament(Client&, uint16_t id);
	void closeSignUp(uint16_t id);
	void startTournament(uint16_t id);
	void removeTournamentObserver(uint16_t id);
	void createTournament(sf::Packet& packet);
	void regularTournaments();
	void saveTournaments();
	void loadTournaments();

	void playChallenge(uint16_t challengeId);

	void getReplay(sf::Packet& packet);

	void pairMatchmaking();
};

#endif
