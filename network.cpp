#include "network.h"
#include <SFML/Network.hpp>
#include <iostream>
#include <thread>
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
		Client newclient;
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
					uploadData.back().uploadTime = uploadClock.getElapsedTime() + sf::seconds(300);
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

void Connections::handle() {
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
					if (sender->room->activePlayers == 0) {
						sender->room->active=false;
						sender->room->round=false;
						sender->room->countdown=0;
					}
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
					else if (sender->room->activePlayers == 1)
						sender->room->active=true;
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
		case 11:
		{
			sf::String name;
			sf::Uint8 max;
			packet >> name >> max;
			lobby.addRoom(name, max);
		}
		break;
		case 100: // UDP packet with gamestate
			sf::Uint16 dataid;
			sf::Uint8 datacount;

			packet >> dataid >> datacount;
			for (int c=0; packet >> extractor.tmp[c]; c++) {}

			for (auto&& client : clients)
				if (client.id == dataid)
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
		break;
	}
}

void Connections::manageRooms() {
	for (auto&& it : lobby.rooms) {
		if (it.active) {
			for (auto&& fromClient : it.clients) {
				if (fromClient->datavalid) {
					for (auto&& toClient : it.clients)
						if (fromClient->id != toClient->id)
							send(*fromClient, *toClient);
					fromClient->datavalid=false;
				}
			}
			if (!it.round) {
				if (it.countdown) {
					if (it.start.getElapsedTime() > sf::seconds(1)) { //2-Packet 
						it.countdown--;
						it.start.restart();
						packet.clear();
						sf::Uint8 packetid = 2;
						packet << packetid;
						packetid = it.countdown;
						packet << packetid;
						send(it, 1);
						if (it.countdown == 0)
							it.startGame();
					}
				}
				else if (it.start.getElapsedTime() > sf::seconds(3)) { //1-Packet
					it.countdown=it.countdownSetting;
					it.start.restart();
					packet.clear();
					sf::Uint8 packetid = 1;
					packet << packetid;
					packetid = it.countdown;
					packet << packetid;
					it.seed1 = rand();
					it.seed2 = rand();
					packet << it.seed1 << it.seed2;
					send(it, 1);
					packet.clear(); //11-Packet
					packetid = 11;
					packet << packetid << it.seed1 << it.seed2;
					send(it, 2);
				}
			}
			else {
				if (it.endround) {
					bool nowinner=true;
					packet.clear();
					sf::Uint8 packetid = 7; // 7-Packet
					packet << packetid;
					for (auto&& winner : it.clients)
						if (winner->alive) {
							winner->position=1;
							send(*winner);
							nowinner=false;

							packet.clear(); // 15-Packet
							packetid = 15;
							packet << packetid << winner->id << winner->position;
							send(it);
							break;
						}
					if (nowinner) {
						packet.clear();
						packetid = 7;
						packet << packetid;
						send(it);
					}
					it.scoreRound();
					it.endRound();
				}
			}
		}
	}
	lobby.removeIdleRooms();
}

void Connections::manageClients() {
	for (auto&& client : clients) {
		if (client.sdataSet)
			if (client.thread.joinable()) {
				client.thread.join();
				client.sdataSet=false;
			}
		if (client.sdataSetFailed)
			if (client.thread.joinable()) {
				client.thread.join();
				client.sdataSetFailed=false;
				client.thread = std::thread(&Client::getData, &client);
			}
		if (client.authresult)
			if (client.thread.joinable()) {
				client.thread.join();
				sf::Uint8 packetid = 9;
				if (client.authresult==1) { //9-Packet nr2
					packet.clear();
					packet << packetid;
					packetid=1;
					std::cout << "Client: " << client.id << " -> ";
					client.id = stoi(client.authpass.toAnsiString());
					std::cout << client.id << std::endl;
					packet << packetid << client.name << client.id;
					send(client);
					client.guest=false;
					bool copyfound=false;
					for (auto it = uploadData.begin(); it != uploadData.end(); it++)
						if (it->id == client.id && !it->sdataPut) {
							std::cout << "Getting data for " << client.id << " from uploadData" << std::endl;
							client.copy(*it);
							copyfound=true;
							it = uploadData.erase(it);
							break;
						}
					if (!copyfound)
						client.thread = std::thread(&Client::getData, &client);
					client.authresult=0;
				}
				else if (client.authresult==2) {
					packet.clear();
					packet << packetid;
					packetid=0;
					packet << packetid;
					send(client);
					client.authresult=0;
				}
				else {
					client.thread = std::thread(&Client::authUser, &client);
					client.authresult=0;
				}
			}
		if (client.incLines>=1) {
			sf::Uint8 amount=0;
			while (client.incLines>=1) {
				amount++;
				client.incLines-=1.0f;
			}
			packet.clear(); // 10-Packet
			sf::Uint8 packetid = 10;
			packet << packetid << amount;
			send(client);
		}
	}
	
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

void Client::authUser() {
	std::cout << "Authing user..." << std::endl;
	sf::Http::Request request("/speedblocks/smftestauth.php", sf::Http::Request::Post);

    sf::String stream = "name=" + name + "&pass=" + authpass;
    request.setBody(stream);

    sf::Http http("http://speedblocks.esy.es");
    sf::Http::Response response = http.sendRequest(request);

    if (response.getStatus() == sf::Http::Response::Ok) {
        if (response.getBody() != "Failed") {
        	authpass = response.getBody();
        	short c = authpass.find('%');
        	name = authpass.substring(c+1, 100);
        	authpass = authpass.substring(0, c);
        	std::cout << "Auth successfull " << std::endl;
        	authresult=1;
        }
        else {
        	std::cout << "Auth failed" << std::endl;
        	authresult=2;
        }
    }
    else {
        std::cout << "Request failed" << std::endl;
        authresult=3;
    }
}

void Client::sendData() {
	sf::Http::Request request("/speedblocks/add.php", sf::Http::Request::Post);

	sf::String tmp = to_string(s_avgBpm);
	unsigned short c = tmp.find('.');
	if (c != sf::String::InvalidPos)
		tmp = tmp.substring(0,c+3);

    sf::String stream = "id="+to_string(id)+"&maxcombo="+to_string(s_maxCombo)+"&maxbpm="+to_string(s_maxBpm);
    stream += "&gamesplayed="+to_string(s_gamesPlayed)+"&avgbpm="+tmp;
    stream += "&gameswon="+to_string(s_gamesWon)+"&rank="+to_string(s_rank)+"&points="+to_string(s_points);
    stream += "&heropoints="+to_string(s_heropoints)+"&totalbpm="+to_string(s_totalBpm);
    stream += "&totalgamesplayed="+to_string(s_totalGames)+"&herorank="+to_string(s_herorank);
    request.setBody(stream);

    sf::Http http("http://speedblocks.esy.es");
    sf::Http::Response response = http.sendRequest(request);

    if (response.getStatus() == sf::Http::Response::Ok) {
        	std::cout << "Client " << (int)id << ": " << response.getBody() << std::endl;
        	sdataSet=true;
    }
    else {
        std::cout << "request failed" << std::endl;
        sdataSetFailed=true;
    }
}

void Client::getData() {
	std::cout << "Getting data for " << (int)id << std::endl;
	sf::Http::Request request("/speedblocks/get.php", sf::Http::Request::Post);

    sf::String stream = "id=" + to_string(id);
    request.setBody(stream);

    sf::Http http("http://speedblocks.esy.es");
    sf::Http::Response response = http.sendRequest(request);

    if (response.getStatus() == sf::Http::Response::Ok) {
    	if (response.getBody().substr(0,5) == "Empty") {
    		std::cout << "No stats found for " << (int)id << std::endl;
    		s_maxCombo = 0; s_maxBpm = 0; s_avgBpm = 0; s_gamesPlayed = 0; s_gamesWon = 0; s_rank = 25; s_points = 0;
    		s_heropoints=0; s_herorank=0; s_totalGames=0; s_totalBpm=0;
    		sdataInit=true;
    		sdataSet=true;
    	}
    	else {
	    	std::string data = response.getBody();
	    	std::cout << data << std::endl;
	    	short c2;
	    	short c = data.find('%');
	    	s_avgBpm = getDataFloat(0,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_gamesPlayed = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_gamesWon = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_heropoints = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_herorank = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_maxBpm = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_maxCombo = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_points = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_rank = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_totalBpm = getDataInt(c2,c,data);
	    	c2=c+1; c=data.find('%',c2);
	    	s_totalGames = getDataInt(c2,c,data);
	    	std::cout << "Data retrieved for " << (int)id << std::endl;
	    	sdataInit=true;
	    	sdataSet=true;
	    }
    }
    else {
        std::cout << "request failed" << std::endl;
        sdataSetFailed = true;
    }
}

int Client::getDataInt(short c2, short c, std::string& data) {
	if (c > c2)
		return stoi(data.substr(c2, c));
	else
		return 0;
}

float Client::getDataFloat(short c2, short c, std::string& data) {
	if (c > c2)
		return stof(data.substr(c2, c));
	else
		return 0;
}

void Client::copy(Client& client) {
	s_maxCombo=client.s_maxCombo; s_maxBpm=client.s_maxBpm; s_rank=client.s_rank;
	s_points=client.s_points; s_heropoints=client.s_heropoints; s_herorank=client.s_herorank;
	s_avgBpm=client.s_avgBpm; s_totalBpm=client.s_totalBpm;
	s_gamesPlayed=client.s_gamesPlayed; s_gamesWon=client.s_gamesWon; s_totalGames=client.s_totalGames;
	sdataInit=true;
	away=false;
}

Client::Client(const Client& client) {
	id = client.id;  room=nullptr; sdataSet=false; sdataSetFailed=false; sdataInit=false; sdataPut=false;
	
	maxCombo = client.maxCombo; position = client.position;
	linesSent = client.linesSent; linesReceived = client.linesReceived; linesBlocked = client.linesBlocked;
	bpm = client.bpm; spm = client.spm;

	s_maxCombo = client.s_maxCombo; s_maxBpm = client.s_maxBpm; s_rank = client.s_rank;
	s_points = client.s_points; s_heropoints = client.s_heropoints; s_herorank = client.s_herorank;
	s_avgBpm = client.s_avgBpm; s_gamesPlayed = client.s_gamesPlayed; s_gamesWon = client.s_gamesWon;
	s_totalGames = client.s_totalGames; s_totalBpm = client.s_totalBpm;
}

void Room::startGame() {
	round=true;
	playersAlive = activePlayers;
	transfearScore();
	leavers.clear();
	adjust.clear();
	endround=false;
	countdown=false;
	for (auto&& client : clients) {
		client->datavalid=false;
		client->datacount=250;
		client->position=0;
		client->linesSent=0;
		client->linesBlocked=0;
		client->garbageCleared=0;
		client->incLines=0;
		client->linesAdjusted=0;
		client->history.clear();
		if (!client->away) {
			client->s_gamesPlayed++;
			client->s_totalGames++;
			client->alive=true;
		}
	}
}

void Room::transfearScore() {
	for (auto&& leaver : leavers) {
		for (auto&& client : conn->clients)
			if (leaver.id == client.id) {
				client.s_points += leaver.s_points;
				if (client.s_points>1000) {
					client.s_points=0;
					client.s_rank--;
				}
				if (client.s_points<-1000) {
					client.s_points=0;
					client.s_rank++;
				}
			}
		for (auto&& client : conn->uploadData)
			if (leaver.id == client.id) {
				client.s_points += leaver.s_points;
				if (client.s_points>1000) {
					client.s_points=0;
					client.s_rank--;
				}
				if (client.s_points<-1000) {
					client.s_points=0;
					client.s_rank++;
				}
			}
	}
}

void Room::endRound() { //6-Packet
	round=false;
	countdown=0;
	roundLenght = start.restart();
	activePlayers = currentPlayers;
	for (auto&& client : clients)
		if (client->away)
			activePlayers--;
	conn->packet.clear();
	sf::Uint8 packetid = 6;
	conn->packet << packetid;
	conn->send(*this);
}

void Room::join(Client& jClient) {
	if (currentPlayers<maxPlayers || maxPlayers == 0) {
		if (!activePlayers)
			countdown=0;
		currentPlayers++;
		activePlayers++;
		if (activePlayers == 2)
			endround=true;
		clients.push_back(&jClient);
		active=true;
		jClient.room = this;
		jClient.alive=false; jClient.maxCombo=0; jClient.position=0; jClient.linesSent=0; jClient.linesReceived=0;
		jClient.linesBlocked=0; jClient.bpm=0; jClient.spm=0; jClient.datavalid=false; jClient.score=0; jClient.incLines=0;
		jClient.away=false;
		jClient.datacount=250;
		std::cout << jClient.id << " joined room " << id << std::endl;
	}
}

void Room::leave(Client& lClient) {
	for (auto it = clients.begin(); it != clients.end(); it++)
		if ((*it)->id == lClient.id) {
			currentPlayers--;
			if (!lClient.away)
				activePlayers--;
			if (lClient.alive) {
				lClient.position=playersAlive;
				playerDied();
			}
			if ((!lClient.s_rank != !hero) && !lClient.guest) {
				leavers.push_back(lClient);
				leavers.back().s_points=0;
			}
			it = clients.erase(it);
			if (activePlayers == 0) {
				active=false;
				round=false;
				countdown=0;
			}
			lClient.incLines = 0;
			lClient.room = nullptr;
			std::cout << lClient.id << " left room " << id << std::endl;

			conn->packet.clear(); //5-Packet
			sf::Uint8 packetid = 5;
			conn->packet << packetid << lClient.id;
			conn->send(*this);
			break;
		}
}

void Room::updatePlayerScore() {
	for (auto&& client : clients) {
		if (client->maxCombo > client->s_maxCombo)
			client->s_maxCombo = client->maxCombo;
		client->s_totalBpm+=client->bpm;
		client->s_avgBpm = (float)client->s_totalBpm / (float)client->s_gamesPlayed;
	}
}

void Room::scoreRound() {
	if (currentPlayers + leavers.size() < 2)
		return;

	short playersinround=0;
	float avgrank=0;
	for (auto&& client : clients) {
		if (client->position)
			client->score += currentPlayers - client->position;
		if (client->position && (!client->s_rank != !hero) && !client->guest) {
			playersinround++;
			avgrank+=client->s_rank;
		}
	}
	for (auto&& client : leavers) {
		playersinround++;
		avgrank+=client.s_rank;
	}
	avgrank/=playersinround; // Determine number of players to be scored & avg rank

	std::cout << "Scoring: " << playersinround << " players, avg rank: " << avgrank << std::endl;

	float* pointcoff = new float[playersinround];
	Client** inround = new Client*[playersinround];

	short lookingfor=0, position=1;
	while (lookingfor<playersinround) {
		for (auto&& client : clients)
			if (position == client->position && (!client->s_rank != !hero) && !client->guest) {
				inround[lookingfor] = client;
				lookingfor++;
				break;
			}
		for (auto&& client : leavers)
			if (position == client.position) {
				inround[lookingfor] = &client;
				lookingfor++;
				break;
			}
		position++;
	}

	for (short i=0; i<playersinround; i++) {
		pointcoff[i] = ((((float)i+1)/(float)playersinround) - 1.0/(float)playersinround  - (1.0-1.0/(float)playersinround)/2.0) * (-1.0/ ((1.0-1.0/(float)playersinround)/2.0) );
		pointcoff[i] += (inround[i]->s_rank - avgrank) * 0.05 + 0.2;
		inround[i]->s_points += 100*pointcoff[i]*(inround[i]->s_rank/5.0);
		if (inround[i]->s_points > 1000) {
			inround[i]->s_rank--;
			inround[i]->s_points=0;
		}
		else if (inround[i]->s_points < -1000) {
			if (inround[i]->s_rank == 25)
				inround[i]->s_points = -1000;
			else {
				inround[i]->s_rank++;
				inround[i]->s_points=0;
			}
		}

		std::cout << (int)inround[i]->id << ": " << pointcoff[i] << " -> " << (int)inround[i]->s_points << " & " << (int)inround[i]->s_rank << std::endl;
	}

	delete[] pointcoff;
	delete[] inround;
}

float eloExpected(float pointsA, float pointsB) { //Gives A's ExpectedPoints
	return 1.0 / (1.0 + pow(10.0, (pointsB - pointsA) / 400.0));
}

// Rnew = Rold + K * (ActualPoints - ExpectedPoints)

void Room::playerDied() {
	Adjust adj;
	adj.amount=0;
	for (auto&& client : clients)
		if (client->linesSent > adj.amount)
			adj.amount = client->linesSent;
	playersAlive--;
	adj.players=playersAlive;
	adjust.push_back(adj);
	if (playersAlive < 2)
		endround=true;
}

void Lobby::sendRoomList(Client& client) {
	conn->packet.clear();
	sf::Uint8 packetid = 16; //16-Packet
	conn->packet << packetid << roomCount;
	for (auto&& room : rooms) {
		conn->packet << room.id << room.name << room.currentPlayers << room.maxPlayers;
	}
	conn->send(client);
}

void Lobby::addRoom(const sf::String& name, short max) {
	Room newroom(conn);
	rooms.push_back(newroom);
	rooms.back().name = name;
	rooms.back().id = idcount;
	rooms.back().maxPlayers = max;
	rooms.back().currentPlayers = 0;
	rooms.back().activePlayers = 0;
	rooms.back().countdownSetting = 3;
	cout << "Adding room " << rooms.back().name.toAnsiString() << " as " << rooms.back().id << endl;
	idcount++;
	roomCount++;
	if (idcount<10)
		idcount=10;

	sf::Uint8 packetid = 17; //17-Packet
	conn->packet.clear();
	conn->packet << packetid << rooms.back().id << name << 1 << rooms.back().maxPlayers;
	for (auto&& client : conn->clients)
		conn->send(client);
}

void Lobby::removeIdleRooms() {
	sf::Uint16 id;
	for (auto it = rooms.begin(); it != rooms.end(); it++)
		if (it->currentPlayers == 0 && it->start.getElapsedTime() > sf::seconds(60) && it->id > 9) {
			cout << "Removing room " << it->name.toAnsiString() << " as " << it->id << endl;
			id = it->id;
			it = rooms.erase(it);
			roomCount--;
			sf::Uint8 packetid = 18; //18-Packet
			conn->packet.clear();
			conn->packet << packetid << id;
			for (auto&& client : conn->clients)
				conn->send(client);
		}
}

void Lobby::setMsg(const sf::String& msg) {
	welcomeMsg = msg;
}