#include "Connections.h"
#include <string>
#include <fstream>
#include "MingwConvert.h"
#include "Tournament.h"
using std::cout;
using std::endl;

Connections::Connections() : tcpPort(21512), sender(nullptr), idcount(60000),
	clientVersion(8), clientCount(0), lobby(this) {
	udpSock.bind(21514); selector.add(udpSock);
}

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
		if (listener.accept(*clients.back().socket) == sf::Socket::Done) {
			std::cout << "Client accepted: " << idcount-1  << std::endl;
			clients.back().address = clients.back().socket->getRemoteAddress();
			clients.back().lastHeardFrom = serverClock.getElapsedTime();
			clients.back().guest=true;
			clients.back().updateStatsTime = serverClock.getElapsedTime();
			clients.back().history.client = &clients.back();
			selector.add(*clients.back().socket);
			clientCount++;
			sendWelcomeMsg();
			lobby.sendTournamentList(clients.back());
			lobby.challengeHolder.sendChallengeList(clients.back());
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
		if (selector.isReady(*it->socket)) {
			status = it->socket->receive(packet);
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
	sendClientLeftServerInfo(client);
	if (!client.guest) {
		uploadData.push_back(client);
		uploadData.back().uploadTime = serverClock.getElapsedTime() + sf::seconds(240);
	}
	if (client.room != nullptr)
		client.room->leave(client);
	if (client.tournament != nullptr)
		client.tournament->removeObserver(client);
	if (client.spectating)
		client.spectating->removeSpectator(client);
	if (client.matchmaking)
		lobby.matchmaking1vs1.removeFromQueue(client);
	selector.remove(*client.socket);
	client.socket->disconnect();
	delete client.socket;
	std::cout << "Client " << client.id << " disconnected" << std::endl;
	clientCount--;
}

void Connections::send(Client& client) {
	status = client.socket->send(packet);
	if (status != sf::Socket::Done)
		std::cout << "Error sending TCP packet to " << (int)client.id << std::endl;
}

void Connections::send(Room& room) {
	for (auto&& it : room.clients) {
		status = it->socket->send(packet);
		if (status != sf::Socket::Done)
			std::cout << "Error sending TCP packet to room " << (int)room.id << std::endl;
	}
}

void Connections::send(Room& room, short type) {
	if (type == 1) { //Send package to everyone who is not away
		for (auto&& it : room.clients) {
			if (!it->away) {
				status = it->socket->send(packet);
				if (status != sf::Socket::Done)
					std::cout << "Error sending TCP packet to room " << (int)room.id << std::endl;
			}
		}
	}
	else if (type == 2) {
		for (auto&& it : room.clients) {
			if (it->away) { //Send package to everyone who is away
				status = it->socket->send(packet);
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

void Connections::sendSignal(sf::Uint8 signalId, int id1, int id2) {
	packet.clear();
	packet << (sf::Uint8)254 << signalId;
	if (id1 > -1)
		packet << (sf::Uint16)id1;
	if (id2 > -1)
		packet << (sf::Uint16)id2;
	for (auto&& client : clients) {
		status = client.socket->send(packet);
		if (status != sf::Socket::Done)
			cout << "Error sending Signal packet to " << (int)client.id << endl;
	}
}

void Connections::sendWelcomeMsg() {
	packet.clear();
	sf::Uint8 packetid = 0;
	packet << packetid << clients.back().id << lobby.welcomeMsg << lobby.roomCount;
	for (auto&& it : lobby.rooms)
		packet << it.id << it.name << it.currentPlayers << it.maxPlayers;
	packet << (sf::Uint16)lobby.matchmaking1vs1.queue.size() << (sf::Uint16)lobby.matchmaking1vs1.playing.size();
	sf::Uint16 adjusterClientCount = clientCount-1;
	packet << adjusterClientCount;
	for (auto&& client : clients)
		packet << client.id << client.name;
	send(clients.back());
}

void Connections::sendAuthResult(sf::Uint8 authresult, Client& client) {
	packet.clear();
	sf::Uint8 packetid = 9;
	packet << packetid;
	if (authresult == 2) {
		for (auto&& client : clients) // Checking for duplicate names, and sending back 4 if found
				if (client.id != sender->id && client.name == sender->name)
					authresult=4;
	}
	packet << authresult;
	if (authresult == 1) {
		packet << client.name << client.id;
	}
	send(client);
}

void Connections::sendChatMsg() {
	//Get msg
	sf::Uint8 type;
	sf::String to = "", msg;
	packet >> type;
	if (type == 3)
		packet >> to;
	packet >> msg;
	//Send it out
	packet.clear();
	sf::Uint8 packetid = 12;
	packet << packetid << type << sender->name << msg;
	if (type == 1) {
		if (sender->room) {
			for (auto&& client : sender->room->clients)
				if (client->id != sender->id)
					send(*client);
			for (auto&& client : sender->room->spectators)
				if (client->id != sender->id)
					send(*client);
		}
		else if (sender->spectating) {
			for (auto&& client : sender->spectating->clients)
				if (client->id != sender->id)
					send(*client);
			for (auto&& client : sender->spectating->spectators)
				if (client->id != sender->id)
					send(*client);
		}
	}
	else if (type == 2) {
		for (auto&& client : clients)
			if (client.id != sender->id)
				send(client);
	}
	else {
		for (auto&& client : clients)
			if (client.name == to) {
				send(client);
				break;
			}
	}
}

void Connections::sendClientJoinedServerInfo(Client& client) {
	packet.clear();
	sf::Uint8 packetid = 20;
	packet << packetid << client.id << client.name;
	for (auto&& otherClient : clients)
		if (otherClient.id != client.id)
			send(otherClient);
}

void Connections::sendClientLeftServerInfo(Client& client) { // MOVE
	packet.clear();
	sf::Uint8 packetid = 21;
	packet << packetid << client.id;
	for (auto&& otherClient : clients)
		if (otherClient.id != client.id)
			send(otherClient);
}

void Connections::validateClient() {
	sf::String name, pass;
	sf::Uint8 guest;
	sf::Uint16 version;
	packet >> version >> guest >> sender->name;
	if (version != clientVersion) {
		sendAuthResult(3, *sender);
		std::cout << "Client tried to connect with wrong client version: " << version << std::endl;
	}
	else if (guest) {
		sendAuthResult(2, *sender);
		sender->stats.rank=25;
		std::cout << "Guest confirmed: " << sender->name.toAnsiString() << std::endl;
		sendClientJoinedServerInfo(*sender);
	}
	else
		sender->thread = new std::thread(&Client::authUser, sender);
}

void Connections::validateUDP() {
	sf::Uint16 clientid;
	packet >> clientid;
	for (auto&& client : clients)
		if (client.id == clientid) {
			if (client.udpPort != udpPort)
				client.udpPort = udpPort;
			client.sendSignal(14);
			cout << "Confirmed UDP port for " << client.id << endl;
			return;
		}
}

void Connections::getGamestate() {
	sf::Uint8 datacount;
	packet >> datacount;
	for (int c=0; packet >> extractor.tmp[c]; c++) {}

	if ((datacount<50 && sender->datacount>200) || sender->datacount<datacount) {
		sender->datacount=datacount;
		sender->data=packet;
		sender->datavalid=true;
		HistoryState history;
		sender->history.states.push_front(history);
		if (sender->history.states.size() > 100)
			sender->history.states.pop_back();
		extractor.extract(sender->history.states.front());
		sender->history.validate();
	}
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
		case 1:
			lobby.getReplay();
		break;
		case 2: //Players authing
			validateClient();
		break;
		case 3: //Player died and sending round-data
			sender->getRoundData();
		break;
		case 4: //Player won and sending round-data
			sender->getWinnerData();
		break;
		case 10: // Chat msg
			sendChatMsg();
		break;
		case 11: //Player creating a room
		{
			sf::String name;
			sf::Uint8 max;
			packet >> name >> max;
			lobby.addRoom(name, max, 3, 3);
		}
		break;
		case 21: // Player created a new tournament
			lobby.createTournament();
		break;
		case 99: // UDP packet to show server the right port
			validateUDP();
		break;
		case 100: // UDP packet with gamestate
			getGamestate();
		break;
		case 102: // Ping packet
			sender->ping.get(serverClock.getElapsedTime(), *sender);
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
		case 0: //Player wanting to join a room
			lobby.joinRequest();
		break;
		case 1: //Player left a room
			if (sender->room != nullptr)
				sender->room->leave(*sender);
		break;
		case 2: //Player sent lines
			if (sender->room != nullptr)
				sender->room->sendLines(*sender);
		break;
		case 3: //Player cleared garbage
		{
			sf::Uint16 amount;
			packet >> amount;
			sender->garbageCleared+=amount;
		}
		break;
		case 4: //Player blocked lines
		{
			sf::Uint16 amount;
			packet >> amount;
			sender->linesBlocked+=amount;
		}
		break;
		case 5: //Player went away
			sender->goAway();
		break;
		case 6: //Player came back
			sender->unAway();
		break;
		case 7: // Player is ready
			if (!sender->ready && !sender->alive && sender->room != nullptr) {
				sender->room->sendSignal(15, sender->id);
				sender->room->sendSignalToSpectators(15, sender->id);
			}
			sender->ready=true;
		break;
		case 8: // Player is not ready
			if (sender->ready && !sender->alive && sender->room != nullptr) {
				sender->room->sendSignal(16, sender->id);
				sender->room->sendSignalToSpectators(16, sender->id);
			}
			sender->ready=false;
		break;
		case 9: // Player signed up for tournament
			lobby.signUpForTournament(*sender);
		break;
		case 10: // Players withdrew from tournament
			lobby.withdrawFromTournament(*sender);
		break;
		case 11: // Player close sign-up for a tournament
			lobby.closeSignUp();
		break;
		case 12: // Players started tournament
			lobby.startTournament();
		break;
		case 13: // Player wants to join a tournament game
			lobby.joinTournamentGame();
		break;
		case 14: // Players closes tournament panel
			lobby.removeTournamentObserver();
		break;
		case 15: // Player requested new tournament list
			lobby.sendTournamentList(*sender);
		break;
		case 16: // Player requested new room list
			lobby.sendRoomList(*sender);
		break;
		case 17: // Player want to play challenge
			lobby.playChallenge();
		break;
		case 18: // Player want to watch challenge replay
			lobby.challengeHolder.sendReplay();
		break;
		case 19: // Players wants to spectate a room
			lobby.joinAsSpectator();
		break;
		case 20: // Player stopped spectating room
			if (sender->spectating)
				sender->spectating->removeSpectator(*sender);
		break;
		case 21: // Player joined matchmaking
			lobby.matchmaking1vs1.addToQueue(*sender, serverClock.getElapsedTime());
		break;
		case 22: // Player left matchmaking
			lobby.matchmaking1vs1.removeFromQueue(*sender);
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
		if (serverClock.getElapsedTime() - it->updateStatsTime > sf::seconds(60) && !it->guest) {
			it->updateStatsTime = serverClock.getElapsedTime();
			it->thread = new std::thread(&Client::sendData, &(*it));
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
			it->thread = new std::thread(&Client::sendData, &(*it));
			it->uploadTime = it->uploadTime + sf::seconds(1000);
		}
		if (it->sdataSet) {
			if (it->thread->joinable()) {
				it->thread->join();
				delete it->thread;
				it->sdataSet=false;
				it->sdataSetFailed=false;
				it = uploadData.erase(it);
			}
		}
		else if (it->sdataSetFailed) {
			if (it->thread->joinable()) {
				it->thread->join();
				delete it->thread;
				it->sdataSet=false;
				it->sdataSetFailed=false;
				it->thread = new std::thread(&Client::sendData, &(*it));
			}
		}
	}
}

void Connections::manageTournaments() {
	for (auto&& tournament : lobby.tournaments) {
		tournament.checkIfStart();
		tournament.checkWaitTime();
		tournament.checkIfScoreWasSent();
	}
	lobby.regularTournaments();
	lobby.saveTournaments();
}

void Connections::manageMatchmaking() {
	while (lobby.matchmaking1vs1.checkQueue(serverClock.getElapsedTime()))
		lobby.pairMatchmaking();
}

bool Connections::getKey() {
	std::string line;
	std::ifstream file ("key");

	if (file.is_open()) {
		getline(file, line);
		serverkey = line;
		getline(file, line);
		challongekey = line;
		return true;
	}
	return false;
}