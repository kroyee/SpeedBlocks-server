#include "Client.h"
#include "Connections.h"
#include "JSONWrap.h"
#include "Room.h"
using std::cout;
using std::endl;

Client::Client(Connections* _conn) : conn(_conn), guest(false), away(false) {
	socket = new sf::TcpSocket; room=nullptr; sdataSet=false; sdataSetFailed=false; sdataInit=false;
	sdataPut=false; incLines=0; tournament = nullptr; spectating=nullptr; pingId=255; pingStart=sf::seconds(0);
	pingTime=sf::seconds(0); sdataPutFailed=false; authresult=0; matchmaking=false; updateStatsTime=sf::seconds(0);
}

void Client::authUser() {
	std::cout << "Authing user..." << std::endl;
	sf::Http::Request request("/hash_check.php", sf::Http::Request::Post);

    sf::String stream = "hash=" + name;
    request.setBody(stream);
    request.setField("Content-Type", "application/x-www-form-urlencoded");

    sf::Http http("http://localhost");
    sf::Http::Response response = http.sendRequest(request);

    if (response.getStatus() == sf::Http::Response::Ok) {
        if (response.getBody() != "Failed") {
        	sf::String authresponse = response.getBody();
        	short c = authresponse.find('%');
        	name = authresponse.substring(c+1, 100);
        	authresponse = authresponse.substring(0, c);
        	std::cout << "Auth successfull: " << id << " -> ";
			id = stoi(authresponse.toAnsiString());
			std::cout << id << std::endl;
        	authresult=1;
        }
        else {
        	std::cout << "Auth failed" << std::endl;
        	std::cout << response.getBody() << std::endl;
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
    jwrap.addPair("key", conn->serverkey);
    jwrap.addClientStats(*this);
    sf::Http::Response response = jwrap.sendPost("put_stats.php");

    if (response.getStatus() == sf::Http::Response::Ok) {
        	std::cout << "Client " << (int)id << ": " << response.getBody() << std::endl;
        	sdataSet=true;
    }
    else {
        std::cout << "sendData failed request" << std::endl;
        sdataPutFailed=true;
    }
}

void Client::getData() {
    JSONWrap jwrap;
    jwrap.addPair("id", id);
    jwrap.addPair("all", 1);
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
	if (sdataSet)
		if (thread->joinable()) {
			thread->join();
			delete thread;
			sdataSet=false;
		}
	if (sdataSetFailed)
		if (thread->joinable()) {
			thread->join();
			delete thread;
			sdataSetFailed=false;
			thread = new std::thread(&Client::getData, this);
		}
	if (sdataPutFailed)
		if (thread->joinable()) {
			thread->join();
			delete thread;
			sdataPutFailed=false;
			thread = new std::thread(&Client::sendData, this);
		}
}

void Client::checkIfAuth() {
	if (authresult) {
		if (thread->joinable()) {
			thread->join();
			delete thread;
			if (authresult==1) {
				conn->sendAuthResult(1, *this);
				guest=false;
				bool copyfound=false;
				for (auto it = conn->uploadData.begin(); it != conn->uploadData.end(); it++)
					if (it->id == id && !it->sdataPut) {
						std::cout << "Getting data for " << id << " from uploadData" << std::endl;
						stats = it->stats;
						sdataInit=true;
						copyfound=true;
						it = conn->uploadData.erase(it);
						break;
					}
				if (!copyfound)
					thread = new std::thread(&Client::getData, this);
				authresult=0;
				conn->sendClientJoinedServerInfo(*this);
			}
			else if (authresult==2) {
				conn->sendAuthResult(0, *this);
				authresult=0;
			}
			else {
				thread = new std::thread(&Client::authUser, this);
				authresult=0;
			}
		}
	}
}

void Client::sendLines() {
	if (incLines>=1) {
		sf::Uint8 amount=0;
		while (incLines>=1) {
			amount++;
			incLines-=1.0f;
		}
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

void Client::getRoundData() {
	if (room == nullptr)
		return;
	alive=false;
	position=room->playersAlive;
	ready=false;
	room->playerDied(*this);
	conn->packet >> maxCombo >> linesSent >> linesReceived >> linesBlocked;
	conn->packet >> bpm >> spm;

	room->sendSignal(13, id, position);
	room->sendSignalToSpectators(13, id, position);
}

void Client::getWinnerData() {
	if (room == nullptr)
		return;
	conn->packet >> maxCombo >> linesSent >> linesReceived >> linesBlocked;
	conn->packet >> bpm >> spm;
	if (room->round) {
		cout << "Ending round now" << endl;
		position=1;
		room->endRound();
		if (room->gamemode >= 20000)
			conn->lobby.challengeHolder.checkResult(*this);
	}
	room->sendRoundScores();
	room->updatePlayerScore();
	if (room->gamemode < 6 && room->gamemode != 3)
		stats.gamesWon++;
	if (bpm > stats.maxBpm && room->roundLenght > sf::seconds(10))
		stats.maxBpm = bpm;
}

void Client::sendPacket(sf::Packet& packet) {
	if (socket->send(packet) != sf::Socket::Done)
		std::cout << "Error sending TCP packet to " << id << std::endl;
}

void Client::sendSignal(sf::Uint8 signalId, int id1, int id2) {
	sf::Packet packet;
	packet << (sf::Uint8)254 << signalId;
	if (id1 > -1)
		packet << (sf::Uint16)id1;
	if (id2 > -1)
		packet << (sf::Uint16)id2;
	if (socket->send(packet) != sf::Socket::Done)
		std::cout << "Error sending Signal packet to " << id << std::endl;
}

void Client::sendJoinRoomResponse(Room& room, sf::Uint16 joinok) {
	sf::Packet packet;
	packet << (sf::Uint8)3 << joinok;
	if (joinok == 1 || joinok == 1000) {
		packet << room.seed1 << room.seed2 << room.currentPlayers;
		for (auto&& client : room.clients)
			packet << client->id << client->name;
	}
	sendPacket(packet);
}