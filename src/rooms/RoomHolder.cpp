#include "RoomHolder.hpp"

void RoomHolder::joinRoom(const NP_JoinRoom& p) {
    for (auto&& room : items)
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

bool RoomHolder::alreadyInside(const Room& room, const Client& joiner) {
    for (auto& client : room.clients)
        if (client->id == joiner.id) return true;
    return false;
}

void RoomHolder::sendRoomList(Client& client) { client.sendPacket(PM::make<NP_RoomList>(*this)); }

void RoomHolder::removeIdleRooms() {
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

void RoomHolder::addRoom(const std::string& name, short max, uint16_t mode, uint8_t delay) {
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

void RoomHolder::addTempRoom(uint16_t mode, Node* game, Tournament* _tournament) {
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