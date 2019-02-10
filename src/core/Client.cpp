#include "Client.h"
#include "AsyncTask.h"
#include "Connections.h"
#include "JSONWrap.h"
#include "NetworkPackets.hpp"
#include "Room.h"

using std::cout;
using std::endl;

void HumanClient::authUser() {
    sf::Http::Request request("/hash_check.php", sf::Http::Request::Post);

    std::string stream = "hash=" + name;
    request.setBody(stream);
    request.setField("Content-Type", "application/x-www-form-urlencoded");

    sf::Http http("http://localhost");
    sf::Http::Response response = http.sendRequest(request);

    if (response.getStatus() == sf::Http::Response::Ok) {
        if (response.getBody() != "Failed") {
            std::string authresponse = response.getBody();
            short c = authresponse.find('%');
            name = authresponse.substr(c + 1, 100);
            authresponse = authresponse.substr(0, c);
            std::cout << "Auth successfull: " << id << " -> ";
            id = stoi(authresponse);
            std::cout << id << std::endl;
            authresult = 1;
        } else {
            std::cout << "Auth failed: " << response.getBody() << std::endl;
            authresult = 2;
        }
    } else {
        std::cout << "Request failed" << std::endl;
        authresult = 2;  // Set this to 3 to revert to resending request if the request failed
    }
}

void Client::sendData() {
    JSONWrap jwrap;

    jwrap.addStatsTable(*this, "1v1");
    if (!sendDataPart(jwrap)) return;

    jwrap.addStatsTable(*this, "ffa");
    if (!sendDataPart(jwrap)) return;

    jwrap.addStatsTable(*this, "hero");
    if (!sendDataPart(jwrap)) return;

    jwrap.addStatsTable(*this, "gstats");
    if (!sendDataPart(jwrap)) return;

    sdataSet = true;
}

bool Client::sendDataPart(JSONWrap& jwrap) {
    sf::Http::Response response = jwrap.sendPost("put_stats.php");

    if (response.getStatus() == sf::Http::Response::Ok) {
        if (response.getBody() != "New records created successfully") std::cout << "Client " << (int)id << ": " << response.getBody() << std::endl;
        return true;
    } else {
        std::cout << "sendData failed request" << std::endl;
        sdataPutFailed = true;
        return false;
    }
}

void Client::getData() {
    JSONWrap jwrap;
    jwrap.addPair("id", id);
    jwrap.addPair("1v1", 1);
    jwrap.addPair("ffa", 1);
    jwrap.addPair("hero", 1);
    jwrap.addPair("tstats", 1);
    jwrap.addPair("gstats", 1);
    sf::Http::Response response = jwrap.sendPost("/get_stats.php");

    if (response.getStatus() == sf::Http::Response::Ok) {
        jwrap.jsonToClientStats(this->stats, response.getBody());
        std::cout << "Data retrieved for " << (int)id << std::endl;
        sdataInit = true;
        sdataSet = true;
    } else {
        std::cout << "getData failed request" << std::endl;
        sdataSetFailed = true;
    }
}

void Client::checkIfStatsSet() {
    if (sdataSet) {
        sdataSet = false;
        if (stats.alert) {
            sendAlert(
                "First time here using the latest version i see. Take the time to check out the Message of the Day under the Server tab, there you can find some tips on "
                "the new GUI and it's features.");
            stats.alert = false;
        }
    }
    if (sdataSetFailed) {
        sdataSetFailed = false;
        AsyncTask::add([&]() { getData(); });
    }
    if (sdataPutFailed) {
        sdataPutFailed = false;
        AsyncTask::add([&]() { sendData(); });
    }
}

static auto& sendAuthResult = Signal<void, uint8_t, Client&>::get("SendAuthResult");
static auto& getUploadData = Signal<bool, HumanClient&>::get("GetUploadData");
static auto& sendClientJoinedServerInfo = Signal<void, HumanClient&>::get("SendClientJoinedServerInfo");

void HumanClient::checkIfAuth() {
    if (authresult) {
        if (authresult == 1) {
            sendAuthResult(1, *this);
            guest = false;
            if (!getUploadData(*this)) AsyncTask::add([&]() { getData(); });
            authresult = 0;
            sendClientJoinedServerInfo(*this);
        } else if (authresult == 2) {
            sendAuthResult(0, *this);
            authresult = 0;
        } else {
            AsyncTask::add([&]() { authUser(); });
            authresult = 0;
        }
    }
}

void Client::sendLines() {
    if (roundStats.incLines >= 1) {
        uint8_t amount = roundStats.incLines;
        roundStats.incLines -= amount;
        sendPacket(PM::make<NP_LinesSent>(amount));
    }
}

void Client::goAway() {
    if (!room || away) return;

    room->activePlayers--;
    alive = false;
    if (room->activePlayers < 2) room->setInactive();
    away = true;
    auto packet = PM::make<NP_ClientAway>(id);
    room->sendPacket(packet);
    room->sendPacketToSpectators(packet);
}

void Client::unAway() {
    if (!room || !away) return;

    room->activePlayers++;
    if (room->activePlayers > 1) room->setActive();
    away = false;
    auto packet = PM::make<NP_ClientNotAway>(id);
    room->sendPacket(packet);
    room->sendPacketToSpectators(packet);
}

static auto& checkChallengeResult = Signal<void, Client&>::get("CheckChallengeResult");

void Client::getRoundData(const NP_GameOver& p) {
    if (room == nullptr || !room->round || !alive) return;
    alive = false;
    roundStats = p;
    roundStats.position = room->playersAlive;
    ready = false;
    room->playerDied(*this);
    sendPositionBpm();

    if (!room->playersAlive) {
        room->endRound();
        if (room->gamemode >= 20000) checkChallengeResult(*this);
        makeWinner();
    }
}

void Client::makeWinner() {
    room->sendRoundScores();
    room->updatePlayerScore();
    room->incrementGamesWon(*this);
    if (roundStats.score.bpm > stats.maxBpm && room->roundLenght > sf::seconds(10)) stats.maxBpm = roundStats.score.bpm;
}

void Client::sendPositionBpm() {
    if (!room) return;

    auto packet = PM::make<NP_PlayerPosition>(id, roundStats.position);
    packet.write_as<NP_AvgBpm>(id, roundStats.score.bpm);
    room->sendPacket(packet);
    room->sendPacketToSpectators(packet);
}

void HumanClient::sendPacket(PM& packet) {
    if (socket.send(packet) != sf::Socket::Done) std::cout << "Error sending TCP packet to " << id << std::endl;
}

void Client::sendJoinRoomResponse(Room& room, uint16_t joinok) {
    NP_JoinResponse response{joinok, room.seed1, room.seed2, {}};
    for (auto& player : room.clients) response.players.push_back({player->id, player->name});
    PM packet;
    packet.write(response);

    sendPacket(packet);
}

void Client::sendAlert(const std::string& msg) { sendPacket(PM::make<NP_Alert>(msg)); }

bool HumanClient::initHuman(sf::TcpListener& listener, sf::SocketSelector& selector, const sf::Time& t) {
    if (listener.accept(socket) != sf::Socket::Done) return false;

    std::cout << "Client accepted: " << id << std::endl;
    address = socket.getRemoteAddress();
    lastHeardFrom = t;
    guest = true;
    updateStatsTime = t;
    history.client = this;
    selector.add(socket);

    return true;
}

void HumanClient::sendGameData(sf::UdpSocket& udp) {
    if (!room) return;

    for (auto& client : room->clients)
        if (client->id != id) client->sendGameDataOut(udp, data);

    for (auto& client : room->spectators) client->sendGameDataOut(udp, data);
}

void HumanClient::sendGameDataOut(sf::UdpSocket& udp, NP_Gamestate& data) {
    PM packet;
    packet.write(data);
    auto status = udp.send(packet, address, udpPort);
    if (status != sf::Socket::Done) std::cout << "Error sending UDP game-data to " << (int)id << std::endl;
}

static auto& sendUDP = Signal<void, HumanClient&, PM&>::get("SendUDP");

void HumanClient::countDown(const sf::Time& t) {
    uint16_t compensate = std::min(ping.getAverage() / 2, ping.getLowest());
    compensate = 3000 - t.asMilliseconds() - compensate;
    auto packet = PM::make<NP_Countdown>(compensate);
    sendUDP(*this, packet);
}

void HumanClient::seed(uint16_t seed1, uint16_t seed2, uint8_t signal_id) {
    sendPacket(PM::make<NP_CountdownStart>(seed1, seed2, static_cast<uint8_t>(signal_id || away)));
}

void HumanClient::endRound() { sendPacket(PM::make<NP_RoundEnd>()); }

void HumanClient::startGame() {
    datavalid = false;
    data.count = 250;
    roundStats.clear();
    history.clear();
    hcp.clear();
    if (!away && room) {
        room->incrementGamesPlayed(*this);
        alive = true;
    }
}
