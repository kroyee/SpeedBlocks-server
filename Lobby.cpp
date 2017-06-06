#include "Lobby.h"
#include "Connections.h"
using std::cout;
using std::endl;

void Lobby::joinRequest() {
	sf::Uint16 id;
	conn->packet >> id;
	if (id < 40000)
		joinRoom(id);
	else
		joinTournament(id);
}

void Lobby::joinRoom(sf::Uint16 roomid) {
	for (auto&& it : rooms)
		if (it.id == roomid) {
			if (alreadyInside(it, *conn->sender))
				return;
			if ((it.currentPlayers < it.maxPlayers || it.maxPlayers == 0) && (conn->sender->sdataInit || conn->sender->guest)) {
				conn->sendPacket3(it, 1);
				conn->sendPacket4(it);
				it.join(*conn->sender);
			}
			else if (conn->sender->sdataInit || conn->sender->guest)
				conn->sendPacket3(it, 2);
			else
				conn->sendPacket3(it, 3);
		}
}

void Lobby::joinTournament(sf::Uint16 tournamentid) {
	for (auto&& tournament : tournaments)
		if (tournamentid == tournament.id) {
			conn->sendPacket23(tournament);
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
							conn->sendPacket3(*game.room, 1);
							conn->sendPacket4(*game.room);
							game.room->join(*conn->sender);
							conn->sendPacket24(game);
							return;
						}
						conn->sendPacket3(*game.room, 4);
						return;
					}
}

bool Lobby::alreadyInside(const Room& room, const Client& joiner) {
	for (auto&& client : room.clients)
		if (client->id == joiner.id)
			return true;
	return false;
}

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
	if (idcount>=40000)
		idcount=10;

	conn->sendPacket17(rooms.back());
}

void Lobby::addTempRoom(sf::Uint8 mode, Node* game, Tournament* _tournament) {
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
	if (tmp_idcount<50000)
		tmp_idcount=50000;
}

void Lobby::removeIdleRooms() {
	for (auto it = rooms.begin(); it != rooms.end(); it++)
		if (it->currentPlayers == 0 && it->start.getElapsedTime() > sf::seconds(60) && it->id > 9) {
			cout << "Removing room " << it->name.toAnsiString() << " as " << it->id << endl;
			conn->sendPacket18(it->id);
			it = rooms.erase(it);
			roomCount--;
		}
	for (auto it = tmp_rooms.begin(); it != tmp_rooms.end(); it++)
		if (it->currentPlayers == 0) {
			cout << "Removing tmp room " << it->id << endl;
			if (it->tournamentGame != nullptr)
				it->tournamentGame->room = nullptr;
			it = tmp_rooms.erase(it);
		}
}

void Lobby::setMsg(const sf::String& msg) {
	welcomeMsg = msg;
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
	if (tourn_idcount >= 50000)
		tourn_idcount = 40000;
	tournamentCount++;
	tournaments.push_back(newTournament);
}

void Lobby::removeTournament(sf::Uint16 id) {
	for (auto it = tournaments.begin(); it != tournaments.end(); it++)
		if (it->id == id) {
			tournaments.erase(it);
			break;
		}
}

void Lobby::sendTournamentList(Client& client) {
	conn->sendPacket22(client);
}

void Lobby::signUpForTournament(Client& client) {
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
							conn->sendSignal(*conn->sender, 0);
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
					if (tournament.players < 3) {
						conn->sendSignal(*conn->sender, 0);
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
			for (auto it = tournament.keepUpdated.begin(); it != tournament.keepUpdated.end(); it++)
				if ((*it)->id == conn->sender->id) {
					tournament.keepUpdated.erase(it);
					return;
				}
}

void Lobby::createTournament() {
	Tournament newTournament(*conn);
	conn->packet >> newTournament.name >> newTournament.sets >> newTournament.rounds;
	newTournament.status = 0;
	newTournament.id = tourn_idcount;
	newTournament.moderator_list.push_back(conn->sender->id);
	tourn_idcount++;
	if (tourn_idcount >= 50000)
		tourn_idcount = 40000;
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
		tournaments.back().setStartingTime(0, 18, 0);
		daily = &tournaments.back();
	}
	else {
		time_t now = time(0);
		struct tm* nowtm = gmtime(&now);
		struct tm* tournamenttm = gmtime(&daily->startingTime);
		if (nowtm->tm_wday != tournamenttm->tm_wday) {
			removeTournament(daily->id);
			daily = nullptr;
		}
	}
}