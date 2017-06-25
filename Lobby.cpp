#include "Lobby.h"
#include "Connections.h"
using std::cout;
using std::endl;

void Lobby::joinRequest() {
	sf::Uint16 id;
	conn->packet >> id;
	if (id < 10000)
		joinRoom(id);
	else if (id < 20000)
		joinTournament(id);
	else
		challengeHolder.sendLeaderboard(id);
}

void Lobby::joinRoom(sf::Uint16 roomid) {
	for (auto&& it : rooms)
		if (it.id == roomid) {
			if (alreadyInside(it, *conn->sender))
				return;
			if ((it.currentPlayers < it.maxPlayers || it.maxPlayers == 0) && (conn->sender->sdataInit || conn->sender->guest)) {
				sendJoinRoomResponse(it, 1);
				it.sendNewPlayerInfo();
				it.join(*conn->sender);
			}
			else if (conn->sender->sdataInit || conn->sender->guest)
				sendJoinRoomResponse(it, 2);
			else
				sendJoinRoomResponse(it, 3);
		}
}

void Lobby::sendJoinRoomResponse(Room& room, sf::Uint16 joinok) {
	conn->packet.clear();
	sf::Uint8 packetid = 3;
	conn->packet << packetid << joinok;
	if (joinok == 1 || 5) {
		conn->packet << room.seed1 << room.seed2 << room.currentPlayers;
		for (auto&& client : room.clients)
			conn->packet << client->id << client->name;
	}
	conn->send(*conn->sender);
}

void Lobby::joinTournament(sf::Uint16 tournamentid) {
	for (auto&& tournament : tournaments)
		if (tournamentid == tournament.id) {
			tournament.sendTournament();
			tournament.addObserver(*conn->sender);
		}
}

void Lobby::joinTournamentGame() {
	sf::Uint16 tId, gId;
	conn->packet >> tId >> gId;
	for (auto&& tournament : tournaments)
		if (tId == tournament.id)
			for (auto&& game : tournament.bracket.games)
				if (gId == game.id)
					if (game.status == 2 || game.status == 3) {
						if (conn->sender->id == game.player1->id || conn->sender->id == game.player2->id) {
							if (game.room == nullptr)
								addTempRoom(4, &game, &tournament);
							if (alreadyInside(*game.room, *conn->sender))
								return;
							sendJoinRoomResponse(*game.room, 1);
							game.room->sendNewPlayerInfo();
							game.room->join(*conn->sender);
							game.sendScore();
							game.resetWaitTimeSent();
							return;
						}
						sendJoinRoomResponse(*game.room, 4);
						return;
					}
}

void Lobby::joinAsSpectator() {
	sf::Uint16 id;
	conn->packet >> id;
	if (id < 10000) {
		for (auto&& room : rooms)
			if (room.id == id) {
				if (room.addSpectator(*conn->sender))
					sendJoinRoomResponse(room, 1000);
				return;
			}
	}
	else if (id < 20000) {
		sf::Uint16 gId;
		conn->packet >> gId;
		for (auto&& tournament : tournaments)
		if (id == tournament.id)
			for (auto&& game : tournament.bracket.games)
				if (gId == game.id)
					if (game.status == 2 || game.status == 3) {
						if (game.room == nullptr)
							addTempRoom(4, &game, &tournament);
						if (!game.room->addSpectator(*conn->sender))
							return;
						sendJoinRoomResponse(*game.room, 1000);
						game.sendScore();
						game.resetWaitTimeSent();
						return;
					}
	}
}

bool Lobby::alreadyInside(const Room& room, const Client& joiner) {
	for (auto&& client : room.clients)
		if (client->id == joiner.id)
			return true;
	return false;
}

void Lobby::sendRoomList(Client& client) {
	conn->packet.clear();
	sf::Uint8 packetid = 16;
	conn->packet << packetid << roomCount;
	for (auto&& room : rooms) {
		conn->packet << room.id << room.name << room.currentPlayers << room.maxPlayers;
	}
	conn->send(client);
}

void Lobby::addRoom(const sf::String& name, short max, sf::Uint16 mode, sf::Uint8 delay) {
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
	if (idcount>=10000)
		idcount=10;
	if (conn->sender != nullptr)
		sendRoomList(*conn->sender);
}

void Lobby::addTempRoom(sf::Uint16 mode, Node* game, Tournament* _tournament) {
	Room newroom(conn);
	tmp_rooms.push_back(newroom);
	tmp_rooms.back().id = tmp_idcount;
	tmp_rooms.back().maxPlayers = 2;
	tmp_rooms.back().currentPlayers = 0;
	tmp_rooms.back().activePlayers = 0;
	tmp_rooms.back().countdownSetting = 3;
	tmp_rooms.back().gamemode = mode;
	tmp_rooms.back().timeBetweenRounds = sf::seconds(0);
	tmp_rooms.back().tournamentGame = game;
	tmp_rooms.back().tournament = _tournament;
	if (game != nullptr)
		game->room = &tmp_rooms.back();
	cout << "Adding tmp room as " << tmp_rooms.back().id << endl;
	tmp_idcount++;
	if (tmp_idcount>=30000)
		tmp_idcount=20000;
}

void Lobby::removeIdleRooms() {
	for (auto it = rooms.begin(); it != rooms.end(); it++)
		if (it->currentPlayers == 0 && it->start.getElapsedTime() > sf::seconds(60) && it->id > 9 && !it->spectators.size()) {
			cout << "Removing room " << it->name.toAnsiString() << " as " << it->id << endl;
			it = rooms.erase(it);
			roomCount--;
		}
	for (auto it = tmp_rooms.begin(); it != tmp_rooms.end(); it++)
		if (it->currentPlayers == 0 && !it->spectators.size()) {
			cout << "Removing tmp room " << it->id << endl;
			if (it->tournamentGame != nullptr)
				it->tournamentGame->room = nullptr;
			it = tmp_rooms.erase(it);
		}
}

void Lobby::setMsg(const sf::String& msg) {
	welcomeMsg = msg;
}

void Lobby::sendTournamentList(Client& client) {
	conn->packet.clear();
	sf::Uint8 packetid = 22;
	conn->packet << packetid << tournamentCount;
	for (auto&& tournament : tournaments) {
		conn->packet << tournament.id << tournament.name << tournament.status << tournament.players;
	}
	conn->send(client);
}

void Lobby::addTournament(const sf::String& name, sf::Uint16 _mod_id) {
	Tournament newTournament(*conn);
	newTournament.rounds = 11;
	newTournament.sets = 1;
	newTournament.status = 0;
	newTournament.id = tourn_idcount;
	newTournament.name = name;
	newTournament.moderator_list.push_back(_mod_id);
	tourn_idcount++;
	if (tourn_idcount >= 20000)
		tourn_idcount = 10000;
	tournamentCount++;
	tournaments.push_back(newTournament);
}

void Lobby::removeTournament(sf::Uint16 id) {
	for (auto it = tournaments.begin(); it != tournaments.end(); it++)
		if (it->id == id) {
			tournaments.erase(it);
			tournamentCount--;
			break;
		}
}

void Lobby::signUpForTournament(Client& client) {
	if (client.id >= 60000) {
		client.sendSignal(2);
		return;
	}
	sf::Uint16 id;
	conn->packet >> id;
	for (auto&& tournament : tournaments)
		if (tournament.id == id) {
			for (auto&& player : tournament.participants)
				if (player.id == client.id)
					return;
			tournament.addPlayer(client);
			tournament.sendParticipantList();
			return;
		}
}

void Lobby::withdrawFromTournament(Client& client) {
	sf::Uint16 id;
	conn->packet >> id;
	for (auto&& tournament : tournaments)
		if (tournament.id == id) {
			if (tournament.removePlayer(client.id))
				tournament.sendParticipantList();
			return;
		}
}

void Lobby::closeSignUp() {
	sf::Uint16 id;
	conn->packet >> id;
	for (auto&& tournament : tournaments)
		if (tournament.id == id) {
			for (auto&& mod : tournament.moderator_list)
				if (mod == conn->sender->id)
					if (tournament.status == 0) {
						if (tournament.players < 3) {
							conn->sender->sendSignal(0);
							return;
						}
						tournament.makeBracket();
						tournament.putPlayersInBracket();
						tournament.status = 1;
						tournament.sendGames();
					}
			return;
		}
}

void Lobby::startTournament() {
	sf::Uint16 id;
	conn->packet >> id;
	for (auto&& tournament : tournaments)
		if (tournament.id == id) {
			for (auto&& mod : tournament.moderator_list)
				if (mod == conn->sender->id) {
					if (tournament.players < 2) {
						conn->sender->sendSignal(0);
						return;
					}
					else
						tournament.startTournament();
				}
			return;
		}
}

void Lobby::removeTournamentObserver() {
	sf::Uint16 id;
	conn->packet >> id;
	for (auto&& tournament : tournaments)
		if (tournament.id == id)
			tournament.removeObserver(*conn->sender);
}

void Lobby::createTournament() {
	if (conn->sender->id >= 60000) {
		conn->sender->sendSignal(2);
		return;
	}
	Tournament newTournament(*conn);
	conn->packet >> newTournament.name >> newTournament.sets >> newTournament.rounds;
	newTournament.status = 0;
	newTournament.id = tourn_idcount;
	newTournament.moderator_list.push_back(conn->sender->id);
	tourn_idcount++;
	if (tourn_idcount >= 20000)
		tourn_idcount = 10000;
	tournamentCount++;
	tournaments.push_back(newTournament);
}

void Lobby::dailyTournament() {
	if (daily == nullptr) {
		time_t now = time(0);
		struct tm* nowtm = gmtime(&now);
		sf::String name;
		switch (nowtm->tm_wday) {
			case 0:
				name = "Sunday ";
			break;
			case 1:
				name = "Monday ";
			break;
			case 2:
				name = "Tuesday ";
			break;
			case 3:
				name = "Wednesday ";
			break;
			case 4:
				name = "Thursday ";
			break;
			case 5:
				name = "Friday ";
			break;
			case 6:
				name = "Saturday ";
			break;
		}
		name += "showdown";
		addTournament(name, 0);
		tournaments.back().setStartingTime(0, 19, 30);
		daily = &tournaments.back();
	}
	else {
		sf::Uint8 oldDay, newDay;
		time_t now = time(0);
		struct tm* nowtm = gmtime(&now);
		oldDay = nowtm->tm_wday;
		nowtm = gmtime(&daily->startingTime);
		newDay = nowtm->tm_wday;
		if (oldDay != newDay) {
			removeTournament(daily->id);
			daily = nullptr;
		}
	}
}

void Lobby::playChallenge() {
	if (conn->sender->room != nullptr)
		return;

	if (conn->sender->guest) {
		conn->sender->sendSignal(2);
		return;
	}

	sf::Uint16 challengeId;
	conn->packet >> challengeId;
	bool valid=false;
	for (auto&& chall : challengeHolder.challenges)
		if (chall.id == challengeId)
			valid=true;

	if (!valid)
		return;

	addTempRoom(challengeId);

	tmp_rooms.back().join(*conn->sender);
	sendJoinRoomResponse(tmp_rooms.back(), tmp_rooms.back().gamemode);
}

void Lobby::getReplay() {
	sf::Uint16 type;
	conn->packet >> type;
	if (type >= 20000)
		challengeHolder.updateResult(*conn->sender, type);
}