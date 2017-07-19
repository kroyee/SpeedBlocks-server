#include "Lobby.h"
#include "Connections.h"
#include <fstream>
using std::cout;
using std::endl;

Lobby::Lobby(Connections* _conn) :
	conn(_conn), roomCount(0), tournamentCount(0), idcount(0),
	tourn_idcount(10000), tmp_idcount(20000), daily(nullptr), weekly(nullptr), monthly(nullptr),
	grandslam(nullptr), challengeHolder(*conn), saveTournamentsTime(sf::seconds(0)), tournamentsUpdated(false) {}

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
	for (auto&& room : rooms)
		if (room.id == roomid) {
			if (alreadyInside(room, *conn->sender))
				return;
			if (room.gamemode == 2 && conn->sender->stats.rank != 0) {
				conn->sender->sendSignal(18);
				return;
			}
			if ((room.currentPlayers < room.maxPlayers || room.maxPlayers == 0) && (conn->sender->sdataInit || conn->sender->guest)) {
				conn->sender->sendJoinRoomResponse(room, 1);
				room.sendNewPlayerInfo(*conn->sender);
				room.join(*conn->sender);
			}
			else if (conn->sender->sdataInit || conn->sender->guest)
				conn->sender->sendJoinRoomResponse(room, 2);
			else
				conn->sender->sendJoinRoomResponse(room, 3);
		}
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
							conn->sender->sendJoinRoomResponse(*game.room, 1);
							game.room->sendNewPlayerInfo(*conn->sender);
							game.room->join(*conn->sender);
							game.sendScore();
							game.resetWaitTimeSent();
							return;
						}
						conn->sender->sendJoinRoomResponse(*game.room, 4);
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
					conn->sender->sendJoinRoomResponse(room, 1000);
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
						conn->sender->sendJoinRoomResponse(*game.room, 1000);
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
	conn->packet << (sf::Uint16)matchmaking1vs1.queue.size() << (sf::Uint16)matchmaking1vs1.playing.size();
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
	newTournament.grade = 5;
	tourn_idcount++;
	if (tourn_idcount >= 20000)
		tourn_idcount = 10000;
	tournamentCount++;
	tournaments.push_back(newTournament);
}

void Lobby::regularTournaments() {
	if (daily == nullptr) {
		time_t now = time(0);
		struct tm* nowtm = gmtime(&now);
		sf::String name;
		switch (nowtm->tm_wday) {
			case 0: name = "Sunday "; break;
			case 1: name = "Monday "; break;
			case 2: name = "Tuesday "; break;
			case 3: name = "Wednesday "; break;
			case 4: name = "Thursday "; break;
			case 5: name = "Friday "; break;
			case 6: name = "Saturday "; break;
		}
		name += "showdown";
		addTournament(name, 0);
		if (nowtm->tm_wday == 0)
			tournaments.back().setStartingTime(0, 17, 30);
		else
			tournaments.back().setStartingTime(0, 19, 30);
		tournaments.back().grade = 4;
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

	if (weekly == nullptr) {
		time_t now = time(0);
		struct tm* nowtm = gmtime(&now);
		addTournament("Weekly brawl", 0);
		if (nowtm->tm_wday == 0)
			tournaments.back().setStartingTime(0, 19, 30);
		else
			tournaments.back().setStartingTime(7-nowtm->tm_wday, 19, 30);
		tournaments.back().grade = 3;
		weekly = &tournaments.back();
	}
	else {
		sf::Uint16 oldDay, newDay;
		time_t now = time(0);
		struct tm* nowtm = gmtime(&now);
		oldDay = nowtm->tm_yday;
		nowtm = gmtime(&weekly->startingTime);
		newDay = nowtm->tm_yday;
		if (oldDay-newDay > 2) {
			removeTournament(weekly->id);
			weekly = nullptr;
		}
	}

	if (monthly == nullptr) {
		time_t now = time(0);
		struct tm* nowtm = gmtime(&now);
		sf::String name;
		switch (nowtm->tm_mon) {
			case 0: name = "January "; break;
			case 1: name = "Febuary "; break;
			case 2: name = "March "; break;
			case 3: name = "April "; break;
			case 4: name = "May "; break;
			case 5: name = "June "; break;
			case 6: name = "July "; break;
			case 7: name = "August "; break;
			case 8: name = "September "; break;
			case 9: name = "October "; break;
			case 10: name = "November "; break;
			case 11: name = "Christmas "; break;
		}
		name += "rumble";
		addTournament(name, 0);
		tournaments.back().setStartingTime(26-nowtm->tm_mday, 19, 00);
		tournaments.back().grade = 2;
		monthly = &tournaments.back();
	}
	else {
		sf::Uint8 oldMonth, newMonth;
		time_t now = time(0);
		struct tm* nowtm = gmtime(&now);
		newMonth = nowtm->tm_mon;
		nowtm = gmtime(&monthly->startingTime);
		oldMonth = nowtm->tm_mon;
		if (oldMonth != newMonth) {
			removeTournament(monthly->id);
			monthly = nullptr;
		}
	}

	if (grandslam == nullptr) {
		time_t now = time(0);
		struct tm* nowtm = gmtime(&now);
		addTournament("Battle of Titans", 0);
		int count = 80-nowtm->tm_yday;
		while (count<0)
			count += 91;
		tournaments.back().setStartingTime(count, 19, 00);
		tournaments.back().grade = 1;
		grandslam = &tournaments.back();
	}
	else {
		sf::Uint16 oldDay, newDay;
		time_t now = time(0);
		struct tm* nowtm = gmtime(&now);
		newDay = nowtm->tm_yday;
		nowtm = gmtime(&grandslam->startingTime);
		oldDay = nowtm->tm_yday;
		if (newDay - oldDay > 30) {
			removeTournament(grandslam->id);
			grandslam = nullptr;
		}
	}

	//Remove grade 5 tournaments that are a day old
	for (auto it = tournaments.begin(); it != tournaments.end(); it++)
		if (it->status == 3 && it->grade == 5) {
			sf::Uint16 oldDay, newDay;
			time_t now = time(0);
			struct tm* nowtm = gmtime(&now);
			newDay = nowtm->tm_yday;
			nowtm = gmtime(&it->startingTime);
			oldDay = nowtm->tm_yday;
			if (newDay != oldDay)
				removeTournament(it->id);
		}
}

void Lobby::saveTournaments() {
	if (conn->serverClock.getElapsedTime() - saveTournamentsTime < sf::seconds(60) || !tournamentsUpdated)
		return;

	system("rm Tournaments/*");

	std::ofstream file("Tournaments/tournament.list");
	if (!file.is_open()) {
		cout << "Failed to save tournaments" << endl;
		return;
	}

	file << daily->name.toAnsiString() << endl << weekly->name.toAnsiString() << endl;
	file << monthly->name.toAnsiString() << endl << grandslam->name.toAnsiString() << endl;
	daily->save();
	weekly->save();
	monthly->save();
	grandslam->save();

	for (auto&& tournament : tournaments){
		bool update=true;
		if (daily)
			if (daily->id == tournament.id)
				update=false;
		if (weekly)
			if (weekly->id == tournament.id)
				update=false;
		if (monthly)
			if (monthly->id == tournament.id)
				update=false;
		if (grandslam)
			if (grandslam->id == tournament.id)
				update=false;

		if (update)
			tournament.save();
	}

	file.close();

	saveTournamentsTime = conn->serverClock.getElapsedTime();
	tournamentsUpdated=false;
}

void Lobby::loadTournaments() {
	std::ifstream file("Tournaments/tournament.list");
	if (!file.is_open()) {
		cout << "Failed to load tournaments" << endl;
		return;
	}

	std::string line;
	int count=-1;
	while (!file.eof()) {
		count++;
		getline(file, line);
		if (line == "")
			continue;
		std::ifstream tfile("Tournaments/" + line);
		if (!tfile.is_open()) {
			cout << "Failed to load tournament " << line << endl;
			continue;
		}

		sf::Uint16 mod, player_id;
		sf::Uint8 grade, rounds, sets;
		sf::String name, player_name;
		time_t startingTime;
		getline(tfile, line); name = line;
		getline(tfile, line); grade = stoi(line);
		getline(tfile, line); startingTime = stol(line);
		getline(tfile, line); rounds = stoi(line);
		getline(tfile, line); sets = stoi(line);
		getline(tfile, line); int c = stoi(line);
		for (int i=0; i<c; i++) {
			getline(tfile, line); mod = stoi(line);
			if (!i) addTournament(name, mod);
			//else add a moderator (no function implemented for it yet)
		}
		getline(tfile, line); c = stoi(line);
		for (int i=0; i<c; i++) {
			getline(tfile, line); player_id = stoi(line);
			getline(tfile, line); player_name = line;
			tournaments.back().addPlayer(player_name, player_id);
		}
		tournaments.back().sets = sets;
		tournaments.back().rounds = rounds;
		tournaments.back().startingTime = startingTime;
		tournaments.back().grade = grade;

		cout << "Count = " << count << endl;
		if (count==0)
			daily = &tournaments.back();
		else if (count==1)
			weekly = &tournaments.back();
		else if (count==2)
			monthly = &tournaments.back();
		else if (count==3)
			grandslam = &tournaments.back();

		tfile.close();
	}
	file.close();
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

	conn->sender->sendJoinRoomResponse(tmp_rooms.back(), tmp_rooms.back().gamemode);
	tmp_rooms.back().join(*conn->sender);
}

void Lobby::getReplay() {
	sf::Uint16 type;
	conn->packet >> type;
	if (type >= 20000)
		challengeHolder.updateResult(*conn->sender, type);
}

void Lobby::pairMatchmaking() {
	addTempRoom(5);
	matchmaking1vs1.player1->sendJoinRoomResponse(tmp_rooms.back(), 1);
	tmp_rooms.back().join(*matchmaking1vs1.player1);

	matchmaking1vs1.player2->sendJoinRoomResponse(tmp_rooms.back(), 1);
	tmp_rooms.back().sendNewPlayerInfo(*matchmaking1vs1.player2);
	tmp_rooms.back().join(*matchmaking1vs1.player2);

	matchmaking1vs1.setPlaying();
}