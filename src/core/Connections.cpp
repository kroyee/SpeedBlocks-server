#include "Connections.h"
#include <string>
#include <fstream>
#include "Tournament.h"
#include "GameSignals.h"
#include "JSONWrap.h"
using std::cout;
using std::endl;

Connections::Connections() : tcpPort(21512), sender(nullptr),
	clientVersion(113), clientCount(0), lobby(*this) {
	udpSock.bind(21514); selector.add(udpSock);

	Net::takePacket(2, &Connections::validateClient, this);
	Net::takePacket(3, [&](sf::Packet& packet){ sender->getRoundData(packet); });
	Net::takePacket(4, [&](sf::Packet& packet){ sender->getWinnerData(packet); });
	Net::takePacket(10, &Connections::sendChatMsg, this);
	Net::takePacket(11, [&](sf::Packet& packet){
		sf::String name;
		uint8_t max;
		packet >> name >> max;
		lobby.addRoom(name, max, 3, 3);
	});
	Net::takePacket(99, &Connections::validateUDP, this);
	Net::takePacket(100, &Connections::getGamestate, this);
	Net::takePacket(102, [&](sf::Packet& packet){ sender->ping.get(serverClock.getElapsedTime(), *sender, packet); });

	Net::takeSignal(1, [&](){ if (sender->room != nullptr) sender->room->leave(*sender); });
	Net::takeSignal(2, [&](uint16_t amount){
		if (sender->room != nullptr)
			sender->room->sendLines(sender->id, sender->roundStats, amount);
	});
	Net::takeSignal(3, [&](uint8_t amount){ sender->roundStats.garbageCleared += amount; });
	Net::takeSignal(4, [&](uint8_t amount){ sender->roundStats.linesBlocked += amount; });
	Net::takeSignal(5, [&](){ sender->goAway(); });
	Net::takeSignal(6, [&](){ sender->unAway(); });
	Net::takeSignal(7, [&](){
		if (!sender->ready && !sender->alive && sender->room != nullptr) {
			sender->room->sendSignal(15, sender->id);
			sender->room->sendSignalToSpectators(15, sender->id);
		}
		sender->ready=true;
	});
	Net::takeSignal(8, [&](){
		if (sender->ready && !sender->alive && sender->room != nullptr) {
			sender->room->sendSignal(16, sender->id);
			sender->room->sendSignalToSpectators(16, sender->id);
		}
		sender->ready=false;
	});

	Net::takeSignal(20, [&](){ if (sender->spectating) sender->spectating->removeSpectator(*sender); });
}

bool Connections::setUpListener() {
	if (listener.listen(tcpPort) == sf::Socket::Done) {
		selector.add(listener);
		return true;
	}
	else
		return false;
}

void Connections::listen() {
	if (selector.wait(sf::milliseconds(100)))
		receive();
}

void Connections::receive() {
	if (selector.isReady(listener)) {
		static uint16_t idcount = 60000;
		clients.emplace_back(this);
		Client& newclient = clients.back();
		newclient.id = idcount;
		idcount++;
		if (listener.accept(*newclient.socket) == sf::Socket::Done) {
			std::cout << "Client accepted: " << idcount-1  << std::endl;
			newclient.address = newclient.socket->getRemoteAddress();
			newclient.lastHeardFrom = serverClock.getElapsedTime();
			newclient.guest=true;
			newclient.updateStatsTime = serverClock.getElapsedTime();
			newclient.history.client = &newclient;
			selector.add(*newclient.socket);
			clientCount++;
			sendWelcomeMsg();
			lobby.sendTournamentList(newclient);
			lobby.challengeHolder.sendChallengeList(newclient);
		}
		else {
			std::cout << "CLient accept failed" << std::endl;
			clients.pop_back();
			idcount--;
		}
		if (idcount<60000)
			idcount=60000;
	}
	if (selector.isReady(udpSock)) {
		sf::Packet packet;
		status = udpSock.receive(packet, udpAdd, udpPort);
		if (status == sf::Socket::Done)
			handlePacket(packet);
	}
	for (auto it = clients.begin(); it != clients.end();  it++)
		if (selector.isReady(*it->socket)) {
			sf::Packet packet;
			status = it->socket->receive(packet);
			if (status == sf::Socket::Done) {
				sender = &(*it);
				sender->lastHeardFrom = serverClock.getElapsedTime();
				handlePacket(packet);
			}
			else if (status == sf::Socket::Disconnected) {
				disconnectClient(*it);
				it = clients.erase(it);
			}
		}
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

/*void Connections::send(Client& client) {
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
}*/

void Connections::send(Client& fromClient, Client& toClient) {
	status = udpSock.send(fromClient.data, toClient.address, toClient.udpPort);
	if (status != sf::Socket::Done)
		std::cout << "Error sending UDP packet from " << (int)fromClient.id << " to " << (int)toClient.id << std::endl;
}

void Connections::sendUDP(Client& client, sf::Packet& packet) {
	status = udpSock.send(packet, client.address, client.udpPort);
	if (status != sf::Socket::Done)
		std::cout << "Error sending UDP packet to " << (int)client.id << std::endl;
}

/*void Connections::sendSignal(uint8_t signalId, int id1, int id2) {
	packet.clear();
	packet << (uint8_t)254 << signalId;
	if (id1 > -1)
		packet << (uint16_t)id1;
	if (id2 > -1)
		packet << (uint16_t)id2;
	for (auto&& client : clients) {
		status = client.socket->send(packet);
		if (status != sf::Socket::Done)
			cout << "Error sending Signal packet to " << (int)client.id << endl;
	}
}*/

void Connections::sendWelcomeMsg() {
	sf::Packet packet;
	uint8_t packetid = 0;
	packet << packetid << clients.back().id << lobby.welcomeMsg << lobby.roomCount;
	for (auto& it : lobby.rooms)
		packet << it->id << it->name << it->currentPlayers << it->maxPlayers;
	packet << (uint16_t)lobby.matchmaking1vs1.queue.size() << (uint16_t)lobby.matchmaking1vs1.playing.size();
	uint16_t adjusterClientCount = clientCount-1;
	packet << adjusterClientCount;
	for (auto&& client : clients)
		packet << client.id << client.name;
	clients.back().sendPacket(packet);
}

void Connections::sendAuthResult(uint8_t authresult, Client& client) {
	sf::Packet packet;
	uint8_t packetid = 9;
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
	client.sendPacket(packet);
}

void Connections::sendChatMsg(sf::Packet& packet) {
	//Get msg
	uint8_t type;
	sf::String to = "", msg;
	packet >> type;
	if (type == 3)
		packet >> to;
	packet >> msg;
	//Send it out
	packet.clear();
	uint8_t packetid = 12;
	packet << packetid << type << sender->name << msg;
	if (type == 1) {
		if (sender->room) {
			for (auto& client : sender->room->clients)
				if (client->id != sender->id)
					client->sendPacket(packet);
			for (auto& client : sender->room->spectators)
				if (client->id != sender->id)
					client->sendPacket(packet);
		}
		else if (sender->spectating) {
			for (auto& client : sender->spectating->clients)
				if (client->id != sender->id)
					client->sendPacket(packet);
			for (auto& client : sender->spectating->spectators)
				if (client->id != sender->id)
					client->sendPacket(packet);
		}
	}
	else if (type == 2) {
		for (auto& client : clients)
			if (client.id != sender->id)
				client.sendPacket(packet);
	}
	else {
		for (auto& client : clients)
			if (client.name == to) {
				client.sendPacket(packet);
				break;
			}
	}
}

void Connections::sendClientJoinedServerInfo(Client& client) {
	sf::Packet packet;
	uint8_t packetid = 20;
	packet << packetid << client.id << client.name;
	for (auto& otherClient : clients)
		if (otherClient.id != client.id)
			otherClient.sendPacket(packet);
}

void Connections::sendClientLeftServerInfo(Client& client) { // MOVE
	sf::Packet packet;
	uint8_t packetid = 21;
	packet << packetid << client.id;
	for (auto& otherClient : clients)
		if (otherClient.id != client.id)
			otherClient.sendPacket(packet);
}

void Connections::validateClient(sf::Packet& packet) {
	sf::String name, pass;
	uint8_t guest;
	uint16_t version;
	packet >> version >> guest >> sender->name;
	if (version != clientVersion) {
		sendAuthResult(3, *sender);
		std::cout << "Client tried to connect with wrong client version: " << version << std::endl;
	}
	else if (guest) {
		sendAuthResult(2, *sender);
		std::cout << "Guest confirmed: " << sender->name.toAnsiString() << std::endl;
		sendClientJoinedServerInfo(*sender);
		sender->sendAlert("First time here using the latest version i see.\nTake the time to check out the Message of the Day under the Server tab, there you can find some tips on the new GUI and it's features.");
	}
	else
		sender->thread = new std::thread(&Client::authUser, sender);
}

void Connections::validateUDP(sf::Packet& packet) {
	uint16_t clientid;
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

void Connections::getGamestate(sf::Packet& packet) {
	uint8_t datacount;
	packet >> datacount;

	if ((datacount<50 && sender->datacount>200) || sender->datacount<datacount) {
		sender->datacount=datacount;
		sender->data=packet;
		sender->datavalid=true;
		HistoryState history;
		sender->history.states.push_front(history);
		if (sender->history.states.size() > 100)
			sender->history.states.pop_back();

		for (int c=0; packet >> extractor.tmp[c]; ++c) {}
		extractor.extract(sender->history.states.front());
		sender->history.validate();
	}
}

void getSignal(sf::Packet &packet) {
	uint8_t signalId;
	uint16_t id1, id2;

	packet >> signalId;
	if (!packet.endOfPacket()) {
			packet >> id1;
		if (!packet.endOfPacket()) {
			packet >> id2;
			if (!Net::passOnSignal(signalId, id1, id2))
				cout << "Error passing on signal " << signalId << "(x,y)" << endl;
			return;
		}
		if (!Net::passOnSignal(signalId, id1))
			cout << "Error passing on signal " << signalId << "(x)" << endl;
		return;
	}
	if (!Net::passOnSignal(signalId))
		cout << "Error passing on signal " << signalId << "()" << endl;
}

void Connections::handlePacket(sf::Packet& packet) {
	packet >> id;
	if (id < 100)
		std::cout << "Packet id: " << (int)id << std::endl;
	else if (id != 254) {
		uint16_t clientid;
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
	else {
		getSignal(packet);
		return;
	}
	Net::passOnPacket(id, packet);
}

void Connections::manageRooms() {
	for (auto&& room : lobby.rooms) {
		if (room->active) {
			room->sendGameData();
			room->makeCountdown();
			room->checkIfRoundEnded();
		}
	}
	for (auto&& room : lobby.tmp_rooms) {
		if (room->active) {
			room->sendGameData();
			room->makeCountdown();
			room->checkIfRoundEnded();
		}
	}
	lobby.removeIdleRooms();
}

void Connections::manageClients() {
	static sf::Time users_online_update = sf::seconds(0);
	JSONWrap jwrap;
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

		if (it->room)
			jwrap.addPair(std::to_string(it->id), it->room->gamemode);
		else
			jwrap.addPair(std::to_string(it->id), 0);
	}

	if (serverClock.getElapsedTime() > users_online_update) {
		users_online_update = serverClock.getElapsedTime() + sf::seconds(60);
		jwrap.addPair("key", serverkey);
		sf::Http::Response response = jwrap.sendPost("/set_usersonline.php");

	    if (response.getStatus() != sf::Http::Response::Ok)
	        std::cout << "set_usersonline failed: " << response.getBody() << std::endl;
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