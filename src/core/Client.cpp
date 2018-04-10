#include "Client.h"
#include "Connections.h"
#include "JSONWrap.h"
#include "Room.h"
#include "AsyncTask.h"
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
        	name = authresponse.substr(c+1, 100);
        	authresponse = authresponse.substr(0, c);
        	std::cout << "Auth successfull: " << id << " -> ";
			id = stoi(authresponse);
			std::cout << id << std::endl;
        	authresult=1;
        }
        else {
        	std::cout << "Auth failed: " << response.getBody() << std::endl;
        	authresult=2;
        }
    }
    else {
        std::cout << "Request failed" << std::endl;
        authresult=2; // Set this to 3 to revert to resending request if the request failed
    }
}

void Client::sendData() {
    JSONWrap jwrap;

    jwrap.addStatsTable(*this, "1v1");
    if (!sendDataPart(jwrap))
    	return;

    jwrap.addStatsTable(*this, "ffa");
    if (!sendDataPart(jwrap))
    	return;

    jwrap.addStatsTable(*this, "hero");
    if (!sendDataPart(jwrap))
    	return;

    jwrap.addStatsTable(*this, "gstats");
    if (!sendDataPart(jwrap))
    	return;

	sdataSet=true;
}

bool Client::sendDataPart(JSONWrap& jwrap) {
	sf::Http::Response response = jwrap.sendPost("put_stats.php");

    if (response.getStatus() == sf::Http::Response::Ok) {
    		if (response.getBody() != "New records created successfully")
        		std::cout << "Client " << (int)id << ": " << response.getBody() << std::endl;
        	return true;
    }
    else {
        std::cout << "sendData failed request" << std::endl;
        sdataPutFailed=true;
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
    	sdataInit=true;
    	sdataSet=true;
    }
    else {
        std::cout << "getData failed request" << std::endl;
        sdataSetFailed = true;
    }
}

void Client::checkIfStatsSet() {
	if (sdataSet) {
		sdataSet=false;
		if (stats.alert) {
			sendAlert("First time here using the latest version i see. Take the time to check out the Message of the Day under the Server tab, there you can find some tips on the new GUI and it's features.");
			stats.alert=false;
		}
	}
	if (sdataSetFailed) {
		sdataSetFailed=false;
		AsyncTask::add([&](){ getData(); });
	}
	if (sdataPutFailed) {
		sdataPutFailed=false;
		AsyncTask::add([&](){ sendData(); });
	}
}

static auto& sendAuthResult = Signal<void, uint8_t, Client&>::get("SendAuthResult");
static auto& getUploadData = Signal<bool, HumanClient&>::get("GetUploadData");
static auto& sendClientJoinedServerInfo = Signal<void, HumanClient&>::get("SendClientJoinedServerInfo");

void HumanClient::checkIfAuth() {
	if (authresult) {
		if (authresult==1) {
			sendAuthResult(1, *this);
			guest=false;
			if (!getUploadData(*this))
				AsyncTask::add([&](){ getData(); });
			authresult=0;
			sendClientJoinedServerInfo(*this);
		}
		else if (authresult==2) {
			sendAuthResult(0, *this);
			authresult=0;
		}
		else {
			AsyncTask::add([&](){ authUser(); });
			authresult=0;
		}
	}
}

void Client::sendLines() {
	if (roundStats.incLines>=1) {
		uint8_t amount = roundStats.incLines;
		roundStats.incLines -= amount;
		sendSignal(9, amount);
	}
}

void Client::goAway() {
	if (!room || away) return;

	room->activePlayers--;
	alive=false;
	if (room->activePlayers < 2)
		room->setInactive();
	away=true;
	room->sendSignal(11, id);
	room->sendSignalToSpectators(11, id);
}

void Client::unAway() {
	if (!room || !away) return;

	room->activePlayers++;
	if (room->activePlayers > 1)
		room->setActive();
	away=false;
	room->sendSignal(12, id);
	room->sendSignalToSpectators(12, id);
}

void Client::getRoundData(sf::Packet& packet) {
	if (room == nullptr)
		return;
	alive=false;
	roundStats.position=room->playersAlive;
	ready=false;
	room->playerDied(*this);
	packet >> roundStats.maxCombo >> roundStats.linesSent >> roundStats.linesReceived;
	packet >> roundStats.linesBlocked >> roundStats.bpm;

	room->sendSignal(13, id, roundStats.position);
	room->sendSignalToSpectators(13, id, roundStats.position);
}

static auto& checkChallengeResult = Signal<void, Client&, sf::Packet&>::get("CheckChallengeResult");

void Client::getWinnerData(sf::Packet& packet) {
	if (room == nullptr)
		return;
	packet >> roundStats.maxCombo >> roundStats.linesSent >> roundStats.linesReceived;
	packet >> roundStats.linesBlocked >> roundStats.bpm;
	if (room->round) {
		roundStats.position=1;
		room->endRound();
		if (room->gamemode >= 20000)
			checkChallengeResult(*this, packet);
	}
	room->sendRoundScores();
	room->updatePlayerScore();
	room->incrementGamesWon(*this);
	if (roundStats.bpm > stats.maxBpm && room->roundLenght > sf::seconds(10))
		stats.maxBpm = roundStats.bpm;
}

void HumanClient::sendPacket(sf::Packet& packet) {
	if (socket.send(packet) != sf::Socket::Done)
		std::cout << "Error sending TCP packet to " << id << std::endl;
}

void HumanClient::sendSignal(uint8_t signalId, int id1, int id2) {
	sf::Packet packet;
	packet << (uint8_t)254 << signalId;
	if (id1 > -1)
		packet << (uint16_t)id1;
	if (id2 > -1)
		packet << (uint16_t)id2;
	if (socket.send(packet) != sf::Socket::Done)
		std::cout << "Error sending Signal packet to " << id << std::endl;
}

void Client::sendJoinRoomResponse(Room& room, uint16_t joinok) {
	sf::Packet packet;
	packet << (uint8_t)3 << joinok;
	if (joinok == 1 || joinok == 1000) {
		packet << room.seed1 << room.seed2 << room.currentPlayers;
		for (auto&& client : room.clients)
			packet << client->id << client->name;
	}
	sendPacket(packet);
}

void Client::sendAlert(const std::string& msg) {
	sf::Packet packet;
	packet << (uint8_t)10 << msg;
	sendPacket(packet);
}

bool HumanClient::initHuman(sf::TcpListener& listener, sf::SocketSelector& selector, const sf::Time& t) {
	if (listener.accept(socket) != sf::Socket::Done)
		return false;

	std::cout << "Client accepted: " << id << std::endl;
	address = socket.getRemoteAddress();
	lastHeardFrom = t;
	guest=true;
	updateStatsTime = t;
	history.client = this;
	selector.add(socket);

	return true;
}

void HumanClient::sendGameData(sf::UdpSocket& udp) {
	if (!room)
		return;

	for (auto& client : room->clients)
		if (client->id != id)
			client->sendGameDataOut(udp, data);

	for (auto& client : room->spectators)
		client->sendGameDataOut(udp, data);
}

void HumanClient::sendGameDataOut(sf::UdpSocket& udp, sf::Packet& packet) {
	auto status = udp.send(packet, address, udpPort);
	if (status != sf::Socket::Done)
		std::cout << "Error sending UDP game-data to " << (int)id << std::endl;
}

static auto& sendUDP = Signal<void, HumanClient&, sf::Packet&>::get("SendUDP");

void HumanClient::countDown(const sf::Time& t) {
	uint16_t compensate = std::min(ping.getAverage()/2, ping.getLowest());
	compensate=3000-t.asMilliseconds()-compensate;
	sf::Packet packet;
	packet << (uint8_t)103 << compensate;
	sendUDP(*this, packet);
}

void HumanClient::seed(uint16_t seed1, uint16_t seed2, uint8_t signal_id) {
	if (signal_id || away)
		signal_id = 10;
	else
		signal_id = 4;

	sendSignal(signal_id, seed1, seed2);
}

void HumanClient::endRound() {
	sendSignal(7);
}

void HumanClient::startGame() {
	datavalid=false;
	datacount=250;
	roundStats.clear();
	history.clear();
	if (!away && room) {
		room->incrementGamesPlayed(*this);
		alive=true;
	}
}
