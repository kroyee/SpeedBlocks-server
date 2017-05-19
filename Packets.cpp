#include "Connections.h"
#include "Client.h"
#include "Room.h"
#include "Lobby.h"

//Welcome packet sent when a new client connects
void Connections::sendPacket0() {
	packet.clear();
	sf::Uint8 packetid = 0;
	packet << packetid << clients.back().id << lobby.welcomeMsg << lobby.roomCount;
	for (auto&& it : lobby.rooms)
		packet << it.id << it.name << it.currentPlayers << it.maxPlayers;
	sf::Uint16 adjusterClientCount = clientCount-1;
	packet << adjusterClientCount;
	for (auto&& client : clients)
		packet << client.id << client.name;
	send(clients.back());
}

//Countdown started for a new round
void Connections::sendPacket1(Room& room, sf::Uint8 countdown) {
	packet.clear();
	sf::Uint8 packetid = 1;
	packet << packetid << countdown << room.seed1 << room.seed2;
	send(room, 1);
}

//Countdown is ongoing, sending current count
void Connections::sendPacket2(Room& room, sf::Uint8 countdown) {
	packet.clear();
	sf::Uint8 packetid = 2;
	packet << packetid << countdown;
	send(room, 1);
}

//Response to client if join room was ok or not
void Connections::sendPacket3(Room& it, sf::Uint8 joinok) {
	packet.clear();
	sf::Uint8 packetid = 3;
	packet << packetid << joinok;
	if (joinok == 1) {
		packet << it.seed1 << it.seed2 << it.currentPlayers;
		for (auto&& inroom : it.clients)
			packet << inroom->id << inroom->name;
	}
	send(*sender);
}

//Informing a room that a new player joined
void Connections::sendPacket4(Room& room) {
	packet.clear();
	sf::Uint8 packetid = 4;
	packet << packetid << sender->id << sender->name;
	send(room);
}

//Inform room that a player left
void Connections::sendPacket5(Room& room, sf::Uint16 id) {
	packet.clear();
	sf::Uint8 packetid = 5;
	packet << packetid << id;
	send(room);
}

//Round ended signal
void Connections::sendPacket6(Room& room) {
	packet.clear();
	sf::Uint8 packetid = 6;
	packet << packetid;
	send(room);
}

//Inform player that he won current round
void Connections::sendPacket7(Room& room, Client* winner) {
	packet.clear();
	sf::Uint8 packetid = 7;
	packet << packetid;
	if (winner == nullptr)
		send(room);
	else
		send(*winner);
}

//Sorting function
bool sortClient(Client* client1, Client* client2) {
	return client1->score > client2->score;
}
//Sending round scores to room
void Connections::sendPacket8() {
	packet.clear();
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
	send(*sender->room);
}

//Sending authresults back to client
void Connections::sendPacket9(sf::Uint8 authresult, Client& client) {
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

// Sending lines to client
void Connections::sendPacket10(Client& client, sf::Uint8 amount) {
	packet.clear();
	sf::Uint8 packetid = 10;
	packet << packetid << amount;
	send(client);
}

// Round started signal, with new seeds to players who are away
void Connections::sendPacket11(Room& room) {
	packet.clear();
	sf::Uint8 packetid = 11;
	packet << packetid << room.seed1 << room.seed2;
	send(room, 2);
}

// Chat message
void Connections::sendPacket12(sf::Uint8 type, const sf::String& to, const sf::String& msg) {
	packet.clear();
	sf::Uint8 packetid = 12;
	packet << packetid << type << sender->name << msg;
	if (type == 1) {
		for (auto&& client : sender->room->clients)
			if (client->id != sender->id)
				send(*client);
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

//Player went away
void Connections::sendPacket13() {
	packet.clear();
	sf::Uint8 packetid = 13;
	packet << packetid << sender->id;
	send(*sender->room);
}

//Player came back
void Connections::sendPacket14() {
	packet.clear();
	sf::Uint8 packetid = 14;
	packet << packetid << sender->id;
	send(*sender->room);
}

//Sending the position of a players to the room
void Connections::sendPacket15(Client& client) {
	packet.clear();
	sf::Uint8 packetid = 15;
	packet << packetid << client.id << client.position;
	send(*client.room);
}

//Send room list
void Connections::sendPacket16(Client& client) {
	sf::Uint8 packetid = 16;
	packet << packetid << lobby.roomCount;
	for (auto&& room : lobby.rooms) {
		packet << room.id << room.name << room.currentPlayers << room.maxPlayers;
	}
	send(client);
}

//New room was added
void Connections::sendPacket17(Room& room) {
	sf::Uint8 packetid = 17;
	packet.clear();
	packet << packetid << room.id << room.name << room.currentPlayers << room.maxPlayers;
	for (auto&& client : clients)
		send(client);
}

//Room was removed
void Connections::sendPacket18(sf::Uint16 id) {
	sf::Uint8 packetid = 18;
	packet.clear();
	packet << packetid << id;
	for (auto&& client : clients)
		send(client);
}

//UDP port was confirmed
void Connections::sendPacket19(Client& client) {
	packet.clear();
	sf::Uint8 packetid = 19;
	packet << packetid;
	send(client);
}

//Client joined the server
void Connections::sendPacket20(Client& client) {
	packet.clear();
	sf::Uint8 packetid = 20;
	packet << packetid << client.id << client.name;
	for (auto&& otherClient : clients)
		if (otherClient.id != client.id)
			send(otherClient);
}

//Client left the server
void Connections::sendPacket21(Client& client) {
	packet.clear();
	sf::Uint8 packetid = 21;
	packet << packetid << client.id;
	for (auto&& otherClient : clients)
		if (otherClient.id != client.id)
			send(otherClient);
}

void Connections::sendPacket102() {
	sendUDP(*sender);
}