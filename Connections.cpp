#include "Connections.h"
#include <string>
#include <fstream>
#include "MingwConvert.h"
using std::cout;
using std::endl;

bool Connections::setUpListener() {
	if (listener.listen(tcpPort) == sf::Socket::Done) {
		selector.add(listener);
		return true;
	}
	else
		return false;
}

bool Connections::listen() {
	if (selector.wait(sf::milliseconds(100)))
		return true;
	else
		return false;
}

bool Connections::receive() {
	if (selector.isReady(listener)) {
		Client newclient(this);
		clients.push_back(newclient);
		clients.back().id = idcount;
		idcount++;
		if (listener.accept(clients.back().socket) == sf::Socket::Done) {
			std::cout << "Client accepted: " << idcount-1  << std::endl;
			clients.back().address = clients.back().socket.getRemoteAddress();
			clients.back().lastHeardFrom = serverClock.getElapsedTime();
			selector.add(clients.back().socket);
			clientCount++;
			sendPacket0();
			lobby.sendTournamentList(clients.back());
		}
		else {
			std::cout << "CLient accept failed" << std::endl;
			clients.pop_back();
			idcount--;
		}
		if (idcount<60000)
			idcount=60000;
	}
	packet.clear();
	if (selector.isReady(udpSock)) {
		status = udpSock.receive(packet, udpAdd, udpPort);
		if (status == sf::Socket::Done)
			return true;
	}
	packet.clear();
	for (auto it = clients.begin(); it != clients.end();  it++)
		if (selector.isReady(it->socket)) {
			status = it->socket.receive(packet);
			if (status == sf::Socket::Done) {
				sender = &(*it);
				sender->lastHeardFrom = serverClock.getElapsedTime();
				return true;
			}
			else if (status == sf::Socket::Disconnected) {
				disconnectClient(*it);
				it = clients.erase(it);
			}
		}
	return false;
}

void Connections::disconnectClient(Client& client) {
	sendPacket21(client);
	if (!client.guest) {
		uploadData.push_back(client);
		uploadData.back().uploadTime = serverClock.getElapsedTime() + sf::seconds(0);
	}
	if (client.room != nullptr)
		client.room->leave(client);
	selector.remove(client.socket);
	client.socket.disconnect();
	std::cout << "Client " << client.id << " disconnected" << std::endl;
	clientCount--;
}

void Connections::send(Client& client) {
	status = client.socket.send(packet);
	if (status != sf::Socket::Done)
		std::cout << "Error sending TCP packet to " << (int)client.id << std::endl;
}

void Connections::send(Room& room) {
	for (auto&& it : room.clients) {
		status = it->socket.send(packet);
		if (status != sf::Socket::Done)
			std::cout << "Error sending TCP packet to room " << (int)room.id << std::endl;
	}
}

void Connections::send(Room& room, short type) {
	if (type == 1) { //Send package to everyone who is not away
		for (auto&& it : room.clients) {
			if (!it->away) {
				status = it->socket.send(packet);
				if (status != sf::Socket::Done)
					std::cout << "Error sending TCP packet to room " << (int)room.id << std::endl;
			}
		}
	}
	else if (type == 2) {
		for (auto&& it : room.clients) {
			if (it->away) { //Send package to everyone who is away
				status = it->socket.send(packet);
				if (status != sf::Socket::Done)
					std::cout << "Error sending TCP packet to room " << (int)room.id << std::endl;
			}
		}
	}
}

void Connections::send(Client& fromClient, Client& toClient) {
	status = udpSock.send(fromClient.data, toClient.address, toClient.udpPort);
	if (status != sf::Socket::Done)
		std::cout << "Error sending UDP packet from " << (int)fromClient.id << " to " << (int)toClient.id << std::endl;
}

void Connections::sendUDP(Client& client) {
	status = udpSock.send(packet, client.address, client.udpPort);
	if (status != sf::Socket::Done)
		std::cout << "Error sending UDP packet to " << (int)client.id << std::endl;
}

void Connections::sendSignal(Client& client, sf::Uint8 signalId, sf::Uint16 id1, sf::Uint16 id2) {
	packet.clear();
	packet << (sf::Uint8)254 << signalId;
	if (id1)
		packet << id1;
	if (id2)
		packet << id2;
	status = client.socket.send(packet);
	if (status != sf::Socket::Done)
		std::cout << "Error sending Signal packet to " << (int)client.id << std::endl;
}

void Connections::handlePacket() {
	packet >> id;
	if (id < 100)
		std::cout << "Packet id: " << (int)id << std::endl;
	else if (id != 254) {
		sf::Uint16 clientid;
		packet >> clientid;
		bool valid=false;
		for (auto&& client : clients)
			if (client.id == clientid && client.address == udpAdd && client.udpPort == udpPort) {
				sender = &client;
				valid=true;
				sender->lastHeardFrom = serverClock.getElapsedTime();
				break;
			}
		if (!valid)
			return;
	}
	switch (id) {
		case 0: //Player wanting to join a room
			lobby.joinRequest();
		break;
		case 1: //Player left a room
			//Moved to signal
		break;
		case 2: //Players authing
		{
			sf::String name, pass;
			sf::Uint8 guest;
			sf::Uint16 version;
			packet >> version >> guest >> sender->name;
			if (version != clientVersion) {
				sendPacket9(3, *sender);
				std::cout << "Client tried to connect with wrong client version: " << version << std::endl;
				sender->guest=true;
			}
			else if (guest) {
				sendPacket9(2, *sender);
				sender->guest=true;
				sender->s_rank=25;
				std::cout << "Guest confirmed: " << sender->name.toAnsiString() << std::endl;
				sendPacket20(*sender);
			}
			else
				sender->thread = std::thread(&Client::authUser, sender);
				
		}
		break;
		case 3: //Player died and sending round-data
		{
			sender->alive=false;
			sender->position=sender->room->playersAlive;
			sender->ready=false;
			sender->room->playerDied();
			packet >> sender->maxCombo >> sender->linesSent >> sender->linesReceived >> sender->linesBlocked;
			packet >> sender->bpm >> sender->spm;

			sendPacket15(*sender);
		}
		break;
		case 4: //Player won and sending round-data
		{
			packet >> sender->maxCombo >> sender->linesSent >> sender->linesReceived >> sender->linesBlocked;
			packet >> sender->bpm >> sender->spm;
			sendPacket8();
			sender->room->updatePlayerScore();
			sender->s_gamesWon++;
			if (sender->bpm > sender->s_maxBpm && sender->room->roundLenght > sf::seconds(10))
				sender->s_maxBpm = sender->bpm;
		}
		break;
		case 5: //Player sent lines
		{
			if (sender->room->playersAlive == 1)
				break;
			sf::Uint8 amount;
			packet >> amount;
			float adjust=0, sending=amount, sent=sender->linesSent, actualSend;
			for (auto&& adj : sender->room->adjust) {
				if (sent>=adj.amount)
					continue;
				if (adj.amount-sent<sending) {
					adjust += (float)(adj.amount-sent) / (float)adj.players;
					sent += (float)(adj.amount-sent) / (float)adj.players;
					sending -= (float)(adj.amount-sent) / (float)adj.players;
				}
				else {
					adjust += sending / (float)adj.players;
					sent += sending / (float)adj.players;
					sending -= sending / (float)adj.players;
				}
			}
			sender->linesSent+=amount;
			sender->linesAdjusted+=adjust;
			actualSend=(float)amount-adjust;
			actualSend/= ((float)sender->room->playersAlive-1.0);
			std::cout << "amount: " << (int)amount << " adjust: " << adjust << " players: " << (int)sender->room->playersAlive << std::endl;
			std::cout << "Sending " << actualSend << " to room from " << sender->id << std::endl;
			for (auto&& client : sender->room->clients)
				if (client->id != sender->id && client->alive)
					client->incLines+=actualSend;
		}
		break;
		case 6: //Player cleared garbage
		{
			sf::Uint8 amount;
			packet >> amount;
			sender->garbageCleared+=amount;
		}
		break;
		case 7: //Player blocked lines
		{
			sf::Uint8 amount;
			packet >> amount;
			sender->linesBlocked+=amount;
		}
		break;
		case 8: //Player went away
			if (sender->room != nullptr) {
				if (sender->away == false) {
					sender->room->activePlayers--;
					if (sender->room->activePlayers < 2)
						sender->room->setInactive();
				}
				sender->away=true;
				sendPacket13();
			}
		break;
		case 9: //Player came back
		{
			if (sender->away)
				if (sender->room != nullptr) {
					sender->room->activePlayers++;
					if (sender->room->activePlayers > 1)
						sender->room->setActive();
				}
			sender->away=false;
			sendPacket14();
		}
		break;
		case 10: // Chat msg
		{
			sf::Uint8 type;
			sf::String to = "", msg;
			packet >> type;
			if (type == 3)
				packet >> to;
			packet >> msg;
			sendPacket12(type, to, msg);
		}
		break;
		case 11: //Player creating a room
		{
			sf::String name;
			sf::Uint8 max;
			packet >> name >> max;
			lobby.addRoom(name, max, 3, 3);
		}
		break;
		case 12: // Player is ready
			if (!sender->ready && !sender->alive)
				sendPacket25();
			sender->ready=true;
		break;
		case 13: // Player is not ready
			if (sender->ready && !sender->alive)
				sendPacket26();
			sender->ready=false;
		break;
		case 14: // Player signed up for tournament
			lobby.signUpForTournament(*sender);
		break;
		case 15: // Players withdrew from tournament
			lobby.withdrawFromTournament(*sender);
		break;
		case 16: // Player close sign-up for a tournament
			lobby.closeSignUp();
		break;
		case 17: // Players started tournament
			lobby.startTournament();
		break;
		case 18: // Player wants to join a tournament game
			lobby.joinTournamentGame();
		break;
		case 19: // Players closes tournament panel
			lobby.removeTournamentObserver();
		break;
		case 20: // Player requested new tournament list
			lobby.sendTournamentList(*sender);
		break;
		case 21: // Player created a new tournament
			lobby.createTournament();
		break;
		case 99: // UDP packet to show server the right port
		{
			sf::Uint16 clientid;
			packet >> clientid;
			for (auto&& client : clients)
				if (client.id == clientid) {
					if (client.udpPort != udpPort)
						client.udpPort = udpPort;
					sendPacket19(client);
					cout << "Confirmed UDP port for " << sender->id << endl;
				}
		}
		break;
		case 100: // UDP packet with gamestate
		{
			sf::Uint8 datacount;

			packet >> datacount;
			for (int c=0; packet >> extractor.tmp[c]; c++) {}

			if ((datacount<50 && sender->datacount>200) || sender->datacount<datacount) {
				sender->datacount=datacount;
				sender->data=packet;
				sender->datavalid=true;
				PlayfieldHistory history;
				sender->history.push_front(history);
				if (sender->history.size() > 100)
					sender->history.pop_back();
				extractor.extract(sender->history.front());
			}
		}
		break;
		case 102: // Ping packet
			sendPacket102();
		break;
		case 254: // Signal packet
			handleSignal();
		break;
	}
}

void Connections::handleSignal() {
	sf::Uint8 signalId;
	packet >> signalId;
	switch (signalId) {
		case 1:
			sender->room->leave(*sender);
		break;
	}
}

void Connections::manageRooms() {
	for (auto&& room : lobby.rooms) {
		if (room.active) {
			room.sendGameData();
			room.makeCountdown();
			room.checkIfRoundEnded();
		}
	}
	for (auto&& room : lobby.tmp_rooms) {
		if (room.active) {
			room.sendGameData();
			room.makeCountdown();
			room.checkIfRoundEnded();
		}
	}
	lobby.removeIdleRooms();
}

void Connections::manageClients() {
	for (auto it = clients.begin(); it != clients.end();  it++) {
		if (serverClock.getElapsedTime() - it->lastHeardFrom > sf::seconds(10)) {
			disconnectClient(*it);
			it = clients.erase(it);
			continue;
		}
		it->checkIfStatsSet();
		it->checkIfAuth();
		it->sendLines();
	}
	
	manageUploadData();
}

void Connections::manageUploadData() {
	for (auto it = uploadData.begin(); it != uploadData.end(); it++) {
		if (serverClock.getElapsedTime() > it->uploadTime) {
			it->sdataPut=true;
			it->thread = std::thread(&Client::sendData, &(*it));
			it->uploadTime = it->uploadTime + sf::seconds(1000);
		}
		if (it->sdataSet) {
			if (it->thread.joinable()) {
				it->thread.join();
				it->sdataSet=false;
				it->sdataSetFailed=false;
				it = uploadData.erase(it);
			}
		}
		else if (it->sdataSetFailed) {
			if (it->thread.joinable()) {
				it->thread.join();
				it->sdataSet=false;
				it->sdataSetFailed=false;
				it->thread = std::thread(&Client::sendData, &(*it));
			}
		}
	}
}

void Connections::manageTournaments() {
	for (auto&& tournament : lobby.tournaments) {
		tournament.checkIfStart();
		tournament.checkWaitTime();
	}
	lobby.dailyTournament();
}

bool Connections::getKey() {
	std::string line;
	std::ifstream file ("key");

	if (file.is_open()) {
		getline(file, line);
		serverkey = line;
		return true;
	}
	return false;
}