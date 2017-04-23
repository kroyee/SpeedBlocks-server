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
			sendWelcome();
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
		status = udpSock.receive(packet, udpAdd, udpPortRec);
		if (status == sf::Socket::Done) {
			gameData=true;
			return true;
		}
	}
	packet.clear();
	for (auto it = clients.begin(); it != clients.end();  it++)
		if (selector.isReady(it->socket)) {
			status = it->socket.receive(packet);
			if (status == sf::Socket::Done) {
				sender = &(*it);
				gameData=false;
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

void Connections::sendWelcome() { //0-Packet
	packet.clear();
	sf::Uint8 packetid = 0;
	packet << packetid << clients.back().id << lobby.welcomeMsg << lobby.roomCount;
	for (auto&& it : lobby.rooms)
		packet << it.id << it.name << it.currentPlayers << it.maxPlayers;
	send(clients.back());
}

bool sortClient(Client* client1, Client* client2) {
	return client1->score > client2->score;
}

void Connections::handlePacket() {
	packet >> id;
	if (id < 100)
		std::cout << "Packet id: " << (int)id << std::endl;
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
						packet.clear();
						sf::Uint8 packetid = 3; //3-Packet
						sf::Uint8 joinok = 1;
						packet << packetid << joinok << it.seed1 << it.seed2 << it.currentPlayers;
						for (auto&& inroom : it.clients)
							packet << inroom->id << inroom->name;
						send(*sender);

						packet.clear(); //4-packet
						packetid = 4;
						packet << packetid << sender->id << sender->name;
						for (auto&& inroom : it.clients)
							send(*inroom);
						it.join(*sender);
					}
					else {
						packet.clear(); //3-Packet
						sf::Uint8 packetid = 3;
						sf::Uint8 joinok = 0;
						packet << packetid << joinok;
						send(*sender);
					}
				}
		}
		break;
		case 1: //Player left a room
			sender->room->leave(*sender);
		break;
		case 2: //Players telling server it's UDP port & authing
		{
			sf::String name, pass;
			sf::Uint8 guest;
			sf::Uint16 version;
			packet >> version >> sender->udpPort >> guest >> sender->name >> sender->authpass;
			if (version != clientVersion) {
				packet.clear(); //9-Packet nr3
				sf::Uint8 packetid = 9;
				packet << packetid;
				packetid=3;
				packet << packetid;
				send(*sender);
				std::cout << "Client tried to connect with wrong client version: " << version << std::endl;
			}
			else if (guest) {
				packet.clear(); //9-Packet nr2
				sf::Uint8 packetid = 9;
				packet << packetid;
				sender->guest=true;
				sender->s_rank=25;
				packetid=2;
				for (auto&& client : clients) // Checking for duplicate names, and sending back 4 if found
					if (client.id != sender->id && client.name == sender->name)
						packetid=4;
				packet << packetid;
				send(*sender);
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
			sender->room->playerDied();
			packet >> sender->maxCombo >> sender->linesSent >> sender->linesReceived >> sender->linesBlocked;
			packet >> sender->bpm >> sender->spm;

			packet.clear(); //15-Packet
			sf::Uint8 packetid = 15;
			packet << packetid << sender->id << sender->position;
			send(*sender->room);
		}
		break;
		case 4: //Player won and sending round-data
		{
			packet >> sender->maxCombo >> sender->linesSent >> sender->linesReceived >> sender->linesBlocked;
			packet >> sender->bpm >> sender->spm;
			packet.clear(); // 8-Packet
			sf::Uint8 packetid = 8, count=0;
			for (auto&& client : sender->room->clients)
				if (client->position)
					count++;
			packet << packetid << count;
			sender->room->clients.sort(&sortClient);
			for (auto&& client : sender->room->clients) {
				if (client->position) {
					packet << client->id << client->maxCombo << client->linesSent << client->linesReceived;
					packet << client->linesBlocked << client->bpm << client->spm << client->s_rank << client->position;
					packet << client->score << client->linesAdjusted;
				}
			}
			sender->room->updatePlayerScore();
			sender->s_gamesWon++;
			if (sender->bpm > sender->s_maxBpm && sender->room->roundLenght > sf::seconds(10))
				sender->s_maxBpm = sender->bpm;
			send(*sender->room);
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
				packet.clear(); //13-Packet
				sf::Uint8 packetid = 13;
				packet << packetid << sender->id;
				send(*sender->room);
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
			packet.clear(); //14-Packet
			sf::Uint8 packetid = 14;
			packet << packetid << sender->id;
			send(*sender->room);
		}
		break;
		case 10: // Chat msg
		{
			sf::Uint8 type, packetid = 12; //12-Packet
			sf::String to, msg;
			packet >> type;
			if (type == 1) {
				packet >> msg;
				packet.clear();
				packet << packetid << type << sender->name << msg;
				for (auto&& client : sender->room->clients)
					if (client->id != sender->id)
						send(*client);
			}
			else if (type == 2) {
				packet >> msg;
				packet.clear();
				packet << packetid << type  << sender->name << msg;
				for (auto&& client : clients)
					if (client.id != sender->id)
						send(client);
			}
			else {
				packet >> to >> msg;
				packet.clear();
				packet << packetid << type << sender->name << msg;
				for (auto&& client : clients)
					if (client.name == to) {
						send(client);
						break;
					}
			}
		}
		break;
		case 11: //Player creating a room
		{
			sf::String name;
			sf::Uint8 max;
			packet >> name >> max;
			lobby.addRoom(name, max, 3);
		}
		break;
		case 100: // UDP packet with gamestate
			sf::Uint16 dataid;
			sf::Uint8 datacount;

			packet >> dataid >> datacount;
			for (int c=0; packet >> extractor.tmp[c]; c++) {}

			for (auto&& client : clients)
				if (client.id == dataid) {
					if (client.udpPort != udpPortRec)
						client.udpPort = udpPortRec;
					if ((datacount<50 && client.datacount>200) || client.datacount<datacount) {
						client.datacount=datacount;
						client.data=packet;
						client.datavalid=true;
						PlayfieldHistory history;
						client.history.push_front(history);
						if (client.history.size() > 100)
							client.history.pop_back();
						extractor.extract(client.history.front());
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