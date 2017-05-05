#include "Connections.h"
#include <string>
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
			selector.add(clients.back().socket);
			clientCount++;
			sendPacket0();
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
				return true;
			}
			else if (status == sf::Socket::Disconnected) {
				if (!it->guest) {
					uploadData.push_back(*it);
					uploadData.back().uploadTime = uploadClock.getElapsedTime() + sf::seconds(0);
				}
				if (it->room != nullptr)
					it->room->leave(*it);
				selector.remove(it->socket);
				it->socket.disconnect();
				std::cout << "Client " << it->id << " disconnected" << std::endl;
				it = clients.erase(it);
				clientCount--;
			}
		}
	return false;
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

void Connections::handlePacket() {
	packet >> id;
	if (id < 100)
		std::cout << "Packet id: " << (int)id << std::endl;
	else {
		sf::Uint16 clientid;
		packet >> clientid;
		bool valid=false;
		for (auto&& client : clients)
			if (client.id == clientid && client.address == udpAdd && client.udpPort == udpPort) {
				sender = &client;
				valid=true;
				break;
			}
		if (!valid)
			return;
	}
	switch (id) {
		case 0: //Player wanting to join a room
		{
			sf::Uint16 roomid;
			packet >> roomid;
			for (auto&& it : lobby.rooms)
				if (it.id == roomid) {
					bool alreadyin=false;
					for (auto&& client : it.clients) //Make sure the client is not already in the room
						if (client->id == sender->id)
							alreadyin=true;
					if (alreadyin)
						break;
					if ((it.currentPlayers < it.maxPlayers || it.maxPlayers == 0) && (sender->sdataInit || sender->guest)) {
						sendPacket3(it, 1);
						sendPacket4(it);
						it.join(*sender);
					}
					else
						sendPacket3(it, 0);
				}
		}
		break;
		case 1: //Player left a room
			sender->room->leave(*sender);
		break;
		case 2: //Players authing
		{
			sf::String name, pass;
			sf::Uint8 guest;
			sf::Uint16 version;
			packet >> version >> guest >> sender->name >> sender->authpass;
			if (version != clientVersion) {
				sendPacket9(3, *sender);
				std::cout << "Client tried to connect with wrong client version: " << version << std::endl;
			}
			else if (guest) {
				sendPacket9(2, *sender);
				sender->guest=true;
				sender->s_rank=25;
				std::cout << "Guest confirmed: " << sender->name.toAnsiString() << std::endl;
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
					if (sender->room->activePlayers == 0)
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
					if (sender->room->activePlayers == 2)
						sender->room->endround=true;
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
		case 101:
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
	}
}

void Connections::manageRooms() {
	for (auto&& it : lobby.rooms) {
		if (it.active) {
			it.sendGameData();
			it.makeCountdown();
			it.checkIfRoundEnded();
		}
	}
	lobby.removeIdleRooms();
}

void Connections::manageClients() {
	for (auto&& client : clients) {
		client.checkIfStatsSet();
		client.checkIfAuth();
		client.sendLines();
	}
	
	manageUploadData();
}

void Connections::manageUploadData() {
	for (auto it = uploadData.begin(); it != uploadData.end(); it++) {
		if (uploadClock.getElapsedTime() > it->uploadTime) {
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