#include "Lobby.h"
#include <fstream>
#include "Connections.h"
#include "GameSignals.h"
#include "NetworkPackets.hpp"
using std::cout;
using std::endl;

Lobby::Lobby(Connections& _conn)
    : conn(_conn),
      roomCount(0),
      tournamentCount(0),
      idcount(0),
      tourn_idcount(10000),
      tmp_idcount(20000),
      daily(nullptr),
      weekly(nullptr),
      monthly(nullptr),
      grandslam(nullptr),
      challengeHolder(conn),
      saveTournamentsTime(sf::seconds(0)),
      tournamentsUpdated(false) {
    PM::handle_packet([&](const NP_Replay& p) {
        if (p.type >= 20000) challengeHolder.updateResult(*conn.sender, p);
    });
    PM::handle_packet([&](const NP_CreateTournament& p) { createTournament(p); });

    PM::handle_packet([&](const NP_JoinRoom& p) { joinRoom(p); });
    PM::handle_packet([&](const NP_TournamentJoin& p) { joinTournament(p); });

    PM::handle_packet([&](const NP_TournamentSignUp& p) { signUpForTournament(*conn.sender, p.tournamentID); });
    PM::handle_packet([&](const NP_TournamentWithdraw& p) { withdrawFromTournament(*conn.sender, p.tournamentID); });
    PM::handle_packet([&](const NP_TournamentCloseSignUp& p) { closeSignUp(p.tournamentID); });
    PM::handle_packet([&](const NP_TournamentStart& p) { startTournament(p.tournamentID); });
    PM::handle_packet([&](const NP_TournamentJoinGame& p) { joinTournamentGame(p.tournamentID, p.gameID); });
    PM::handle_packet([&](const NP_TournamentLeftPanel& p) { removeTournamentObserver(p.tournamentID); });
    PM::handle_packet<NP_TournamentListRefresh>([&]() { sendTournamentList(*conn.sender); });
    PM::handle_packet<NP_RoomListRefresh>([&]() { sendRoomList(*conn.sender); });
    PM::handle_packet([&](const NP_ChallengePlay& p) { playChallenge(p.challengeID); });

    PM::handle_packet([&](const NP_SpectatorJoin& p) { joinAsSpectator(p.tournamentID, p.roomID); });

    PM::handle_packet<NP_MatchmakingJoin>([&]() { matchmaking1vs1.addToQueue(*conn.sender, conn.serverClock.getElapsedTime()); });
    PM::handle_packet<NP_MatchmakingLeave>([&]() { matchmaking1vs1.removeFromQueue(*conn.sender); });
}

void Lobby::joinRoom(const NP_JoinRoom& p) {
    for (auto&& room : rooms)
        if (room->id == p.roomID) {
            if (alreadyInside(*room, *conn.sender)) return;
            if (room->gamemode == 2 && conn.sender->stats.ffaRank != 0) {
                conn.sender->sendPacket(PM::make<NP_NotHero>());
                return;
            }
            if ((room->currentPlayers < room->maxPlayers || room->maxPlayers == 0) && (conn.sender->sdataInit || conn.sender->guest)) {
                conn.sender->sendJoinRoomResponse(*room, 1);
                room->sendNewPlayerInfo(*conn.sender);
                room->join(*conn.sender);
            } else if (conn.sender->sdataInit || conn.sender->guest)
                conn.sender->sendJoinRoomResponse(*room, 2);
            else
                conn.sender->sendJoinRoomResponse(*room, 3);
        }
}

void Lobby::joinTournament(const NP_TournamentJoin& p) {
    for (auto&& tournament : tournaments)
        if (p.tournamentID == tournament.id) {
            tournament.sendTournament();
            tournament.addObserver(*conn.sender);
        }
}

void Lobby::joinTournamentGame(uint16_t tId, uint16_t gId) {
    for (auto& tournament : tournaments)
        if (tId == tournament.id)
            for (auto& game : tournament.bracket.games)
                if (gId == game.id)
                    if (game.status == 2 || game.status == 3) {
                        if (conn.sender->id == game.player1->id || conn.sender->id == game.player2->id) {
                            if (game.room == nullptr) addTempRoom(4, &game, &tournament);
                            if (alreadyInside(*game.room, *conn.sender)) return;
                            conn.sender->sendJoinRoomResponse(*game.room, 1);
                            game.room->sendNewPlayerInfo(*conn.sender);
                            game.room->join(*conn.sender);
                            game.sendScore();
                            game.resetWaitTimeSent();
                            return;
                        }
                        conn.sender->sendJoinRoomResponse(*game.room, 4);
                        return;
                    }
}

void Lobby::joinAsSpectator(uint16_t id, uint16_t gId) {
    if (id < 10000) {
        for (auto&& room : rooms)
            if (room->id == id) {
                if (room->addSpectator(*conn.sender)) conn.sender->sendJoinRoomResponse(*room, 1000);
                return;
            }
    } else if (id < 20000) {
        for (auto& tournament : tournaments)
            if (id == tournament.id)
                for (auto& game : tournament.bracket.games)
                    if (gId == game.id)
                        if (game.status == 2 || game.status == 3) {
                            if (game.room == nullptr) addTempRoom(4, &game, &tournament);
                            if (!game.room->addSpectator(*conn.sender)) return;
                            conn.sender->sendJoinRoomResponse(*game.room, 1000);
                            game.sendScore();
                            game.resetWaitTimeSent();
                            return;
                        }
    }
}

bool Lobby::alreadyInside(const Room& room, const Client& joiner) {
    for (auto& client : room.clients)
        if (client->id == joiner.id) return true;
    return false;
}

void Lobby::sendRoomList(Client& client) { client.sendPacket(PM::make<NP_RoomList>(*this)); }

void Lobby::addRoom(const std::string& name, short max, uint16_t mode, uint8_t delay) {
    if (mode == 1)
        rooms.emplace_back(new FFARoom(conn));
    else if (mode == 2)
        rooms.emplace_back(new HeroRoom(conn));
    else if (mode == 3)
        rooms.emplace_back(new CasualRoom(conn));
    else if (mode == 4)
        rooms.emplace_back(new TournamentRoom(conn));
    else if (mode == 5)
        rooms.emplace_back(new VSRoom(conn));
    else
        rooms.emplace_back(new ChallengeRoom(conn, mode));
    rooms.back()->name = name;
    rooms.back()->id = idcount;
    rooms.back()->maxPlayers = max;
    rooms.back()->timeBetweenRounds = sf::seconds(delay);
    cout << "Adding room " << rooms.back()->name << " as " << rooms.back()->id << endl;
    idcount++;
    roomCount++;
    if (idcount >= 10000) idcount = 10;
    if (conn.sender != nullptr) sendRoomList(*conn.sender);
}

void Lobby::addTempRoom(uint16_t mode, Node* game, Tournament* _tournament) {
    if (mode == 1)
        tmp_rooms.emplace_back(new FFARoom(conn));
    else if (mode == 2)
        tmp_rooms.emplace_back(new HeroRoom(conn));
    else if (mode == 3)
        tmp_rooms.emplace_back(new CasualRoom(conn));
    else if (mode == 4)
        tmp_rooms.emplace_back(new TournamentRoom(conn));
    else if (mode == 5)
        tmp_rooms.emplace_back(new VSRoom(conn));
    else
        tmp_rooms.emplace_back(new ChallengeRoom(conn, mode));
    tmp_rooms.back()->id = tmp_idcount;
    tmp_rooms.back()->maxPlayers = 2;
    tmp_rooms.back()->currentPlayers = 0;
    tmp_rooms.back()->activePlayers = 0;
    tmp_rooms.back()->timeBetweenRounds = sf::seconds(0);
    tmp_rooms.back()->tournamentGame = game;
    tmp_rooms.back()->tournament = _tournament;
    if (game != nullptr) game->room = tmp_rooms.back().get();
    cout << "Adding tmp room as " << tmp_rooms.back()->id << endl;
    tmp_idcount++;
    if (tmp_idcount >= 30000) tmp_idcount = 20000;
}

void Lobby::removeIdleRooms() {
    for (auto it = rooms.begin(); it != rooms.end(); it++)
        if ((*it)->currentPlayers == 0 && (*it)->start.getElapsedTime() > sf::seconds(60) && (*it)->id > 9 && !(*it)->spectators.size()) {
            cout << "Removing room " << (*it)->name << " as " << (*it)->id << endl;
            it = rooms.erase(it);
            roomCount--;
        }
    for (auto it = tmp_rooms.begin(); it != tmp_rooms.end(); it++)
        if ((*it)->currentPlayers == 0 && !(*it)->spectators.size()) {
            cout << "Removing tmp room " << (*it)->id << endl;
            if ((*it)->tournamentGame != nullptr) (*it)->tournamentGame->room = nullptr;
            it = tmp_rooms.erase(it);
        }
}

void Lobby::setMsg(const std::string& msg) { welcomeMsg = msg; }

void Lobby::sendTournamentList(Client& client) {
    PM packet;
    NP_TournamentList list;
    for (auto& t : tournaments) list.tournaments.push_back({t.id, t.name, t.status, t.players});
    packet.write(list);
    client.sendPacket(packet);
}

void Lobby::addTournament(const std::string& name, uint16_t _mod_id) {
    Tournament newTournament(conn);
    newTournament.rounds = 11;
    newTournament.sets = 1;
    newTournament.status = 0;
    newTournament.id = tourn_idcount;
    newTournament.name = name;
    newTournament.moderator_list.push_back(_mod_id);
    tourn_idcount++;
    if (tourn_idcount >= 20000) tourn_idcount = 10000;
    tournamentCount++;
    tournaments.push_back(newTournament);
}

void Lobby::removeTournament(uint16_t id) {
    for (auto it = tournaments.begin(); it != tournaments.end(); it++)
        if (it->id == id) {
            tournaments.erase(it);
            tournamentCount--;
            break;
        }
}

void Lobby::signUpForTournament(HumanClient& client, uint16_t id) {
    if (client.guest) {
        client.sendPacket(PM::make<NP_NotAsGuest>());
        return;
    }
    for (auto&& tournament : tournaments)
        if (tournament.id == id) {
            for (auto&& player : tournament.participants)
                if (player.id == client.id) return;
            if (tournament.status != 0) return;
            tournament.addPlayer(client);
            tournament.sendParticipantList();
            return;
        }
}

void Lobby::withdrawFromTournament(Client& client, uint16_t id) {
    for (auto&& tournament : tournaments)
        if (tournament.id == id) {
            if (tournament.removePlayer(client.id)) tournament.sendParticipantList();
            return;
        }
}

void Lobby::closeSignUp(uint16_t id) {
    for (auto&& tournament : tournaments)
        if (tournament.id == id) {
            for (auto& mod : tournament.moderator_list)
                if (mod == conn.sender->id)
                    if (tournament.status == 0) {
                        if (tournament.players < 3) {
                            conn.sender->sendPacket(PM::make<NP_TournamentNotEnough>());
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

void Lobby::startTournament(uint16_t id) {
    for (auto&& tournament : tournaments)
        if (tournament.id == id) {
            for (auto&& mod : tournament.moderator_list)
                if (mod == conn.sender->id) {
                    if (tournament.players < 2) {
                        conn.sender->sendPacket(PM::make<NP_TournamentNotEnough>());
                        return;
                    } else
                        tournament.startTournament();
                }
            return;
        }
}

void Lobby::removeTournamentObserver(uint16_t id) {
    for (auto&& tournament : tournaments)
        if (tournament.id == id) tournament.removeObserver(*conn.sender);
}

void Lobby::createTournament(const NP_CreateTournament& p) {
    if (conn.sender->guest) {
        conn.sender->sendPacket(PM::make<NP_NotAsGuest>());
        return;
    }
    Tournament newTournament(conn);
    newTournament.name = p.name;
    newTournament.sets = p.sets;
    newTournament.rounds = p.rounds;
    newTournament.status = 0;
    newTournament.id = tourn_idcount;
    newTournament.moderator_list.push_back(conn.sender->id);
    newTournament.grade = 5;
    tourn_idcount++;
    if (tourn_idcount >= 20000) tourn_idcount = 10000;
    tournamentCount++;
    tournaments.push_back(newTournament);
}

void Lobby::regularTournaments() {
    if (daily == nullptr) {
        time_t now = time(0);
        struct tm* nowtm = gmtime(&now);
        std::string name;
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
        if (nowtm->tm_wday == 0)
            tournaments.back().setStartingTime(0, 17, 30);
        else
            tournaments.back().setStartingTime(0, 19, 30);
        tournaments.back().grade = 4;
        daily = &tournaments.back();
    } else {
        uint8_t oldDay, newDay;
        time_t now = time(0);
        struct tm* nowtm = gmtime(&now);
        oldDay = nowtm->tm_wday;
        time_t start = daily->startingTime;
        nowtm = gmtime(&start);
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
            tournaments.back().setStartingTime(7 - nowtm->tm_wday, 19, 30);
        tournaments.back().grade = 3;
        weekly = &tournaments.back();
    } else {
        uint16_t oldDay, newDay;
        time_t now = time(0);
        struct tm* nowtm = gmtime(&now);
        oldDay = nowtm->tm_yday;
        time_t start = weekly->startingTime;
        nowtm = gmtime(&start);
        newDay = nowtm->tm_yday;
        if (oldDay - newDay > 2) {
            removeTournament(weekly->id);
            weekly = nullptr;
        }
    }

    if (monthly == nullptr) {
        time_t now = time(0);
        struct tm* nowtm = gmtime(&now);
        std::string name;
        switch (nowtm->tm_mon) {
            case 0:
                name = "January ";
                break;
            case 1:
                name = "Febuary ";
                break;
            case 2:
                name = "March ";
                break;
            case 3:
                name = "April ";
                break;
            case 4:
                name = "May ";
                break;
            case 5:
                name = "June ";
                break;
            case 6:
                name = "July ";
                break;
            case 7:
                name = "August ";
                break;
            case 8:
                name = "September ";
                break;
            case 9:
                name = "October ";
                break;
            case 10:
                name = "November ";
                break;
            case 11:
                name = "Christmas ";
                break;
        }
        name += "rumble";
        addTournament(name, 0);
        tournaments.back().setStartingTime(26 - nowtm->tm_mday, 19, 00);
        tournaments.back().grade = 2;
        monthly = &tournaments.back();
    } else {
        uint8_t oldMonth, newMonth;
        time_t now = time(0);
        struct tm* nowtm = gmtime(&now);
        newMonth = nowtm->tm_mon;
        time_t start = monthly->startingTime;
        nowtm = gmtime(&start);
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
        int count = 80 - nowtm->tm_yday;
        while (count < 0) count += 91;
        tournaments.back().setStartingTime(count, 19, 00);
        tournaments.back().grade = 1;
        grandslam = &tournaments.back();
    } else {
        uint16_t oldDay, newDay;
        time_t now = time(0);
        struct tm* nowtm = gmtime(&now);
        newDay = nowtm->tm_yday;
        time_t start = grandslam->startingTime;
        nowtm = gmtime(&start);
        oldDay = nowtm->tm_yday;
        if (newDay - oldDay > 30) {
            removeTournament(grandslam->id);
            grandslam = nullptr;
        }
    }

    // Remove grade 5 tournaments that are a day old
    for (auto it = tournaments.begin(); it != tournaments.end(); it++)
        if (it->status == 3 && it->grade == 5) {
            uint16_t oldDay, newDay;
            time_t now = time(0);
            struct tm* nowtm = gmtime(&now);
            newDay = nowtm->tm_yday;
            time_t start = it->startingTime;
            nowtm = gmtime(&start);
            oldDay = nowtm->tm_yday;
            if (newDay != oldDay) removeTournament(it->id);
        }
}

void Lobby::saveTournaments() {
    if (conn.serverClock.getElapsedTime() - saveTournamentsTime < sf::seconds(60) || !tournamentsUpdated) return;

    system("rm Tournaments/*");

    std::ofstream file("Tournaments/tournament.list");
    if (!file.is_open()) {
        cout << "Failed to save tournaments" << endl;
        return;
    }

    file << daily->name << endl << weekly->name << endl;
    file << monthly->name << endl << grandslam->name << endl;
    daily->save();
    weekly->save();
    monthly->save();
    grandslam->save();

    for (auto&& tournament : tournaments) {
        bool update = true;
        if (daily)
            if (daily->id == tournament.id) update = false;
        if (weekly)
            if (weekly->id == tournament.id) update = false;
        if (monthly)
            if (monthly->id == tournament.id) update = false;
        if (grandslam)
            if (grandslam->id == tournament.id) update = false;

        if (update) tournament.save();
    }

    file.close();

    saveTournamentsTime = conn.serverClock.getElapsedTime();
    tournamentsUpdated = false;
}

void Lobby::loadTournaments() {
    std::ifstream file("Tournaments/tournament.list");
    if (!file.is_open()) {
        cout << "Failed to load tournaments" << endl;
        return;
    }

    std::string line;
    int count = -1;
    while (!file.eof()) {
        count++;
        getline(file, line);
        if (line == "") continue;
        std::ifstream tfile("Tournaments/" + line);
        if (!tfile.is_open()) {
            cout << "Failed to load tournament " << line << endl;
            continue;
        }

        uint16_t mod, player_id;
        uint8_t grade, rounds, sets;
        std::string name, player_name;
        time_t startingTime;
        getline(tfile, line);
        name = line;
        getline(tfile, line);
        grade = stoi(line);
        getline(tfile, line);
        startingTime = stol(line);
        if (startingTime < time(0)) {
            tfile.close();
            continue;
        }
        getline(tfile, line);
        rounds = stoi(line);
        getline(tfile, line);
        sets = stoi(line);
        getline(tfile, line);
        int c = stoi(line);
        for (int i = 0; i < c; i++) {
            getline(tfile, line);
            mod = stoi(line);
            if (!i) addTournament(name, mod);
            // else add a moderator (no function implemented for it yet)
        }
        getline(tfile, line);
        c = stoi(line);
        for (int i = 0; i < c; i++) {
            getline(tfile, line);
            player_id = stoi(line);
            getline(tfile, line);
            player_name = line;
            tournaments.back().addPlayer(player_name, player_id);
        }
        tournaments.back().sets = sets;
        tournaments.back().rounds = rounds;
        tournaments.back().startingTime = startingTime;
        tournaments.back().grade = grade;

        if (count == 0)
            daily = &tournaments.back();
        else if (count == 1)
            weekly = &tournaments.back();
        else if (count == 2)
            monthly = &tournaments.back();
        else if (count == 3)
            grandslam = &tournaments.back();

        tfile.close();
    }
    file.close();
}

void Lobby::playChallenge(uint16_t challengeId) {
    if (conn.sender->room != nullptr) return;

    if (conn.sender->guest) {
        conn.sender->sendPacket(PM::make<NP_NotAsGuest>());
        return;
    }

    if (std::none_of(challengeHolder.challenges.begin(), challengeHolder.challenges.end(), [&](std::unique_ptr<Challenge>& chall) { return chall->id == challengeId; }))
        return;

    addTempRoom(challengeId);

    conn.sender->sendJoinRoomResponse(*tmp_rooms.back(), tmp_rooms.back()->gamemode);
    tmp_rooms.back()->join(*conn.sender);
}

void Lobby::pairMatchmaking() {
    addTempRoom(5);
    matchmaking1vs1.player1->sendJoinRoomResponse(*tmp_rooms.back(), 1);
    tmp_rooms.back()->join(*matchmaking1vs1.player1);

    matchmaking1vs1.player2->sendJoinRoomResponse(*tmp_rooms.back(), 1);
    tmp_rooms.back()->sendNewPlayerInfo(*matchmaking1vs1.player2);
    tmp_rooms.back()->join(*matchmaking1vs1.player2);

    matchmaking1vs1.setPlaying();
}
