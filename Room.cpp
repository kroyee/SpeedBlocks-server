#include "Room.h"
#include "Connections.h"
#include "Tournament.h"
using std::cout;
using std::endl;

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

void Room::startCountdown() {
	countdown=countdownSetting;
	playersAlive=activePlayers;
	start.restart();
	seed1 = rand();
	seed2 = rand();
	sendSignalToActive(4, seed1, seed2);
	sendSignalToAway(10, seed1, seed2);
	sendSignalToSpectators(10, seed1, seed2);
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

void Room::endRound() {
	if (round)
		for (auto&& client : clients)
			client->ready=false;
	round=false;
	countdown=0;
	roundLenght = start.restart();
	sendSignal(7);
}

void Room::join(Client& jClient) {
	if (currentPlayers<maxPlayers || maxPlayers == 0) {
		if (jClient.spectating)
			jClient.spectating->removeSpectator(jClient);
		if (!activePlayers)
			countdown=0;
		currentPlayers++;
		activePlayers++;
		if (activePlayers > 1 || gamemode >= 20000)
			setActive();
		clients.push_back(&jClient);
		jClient.room = this;
		jClient.alive=false; jClient.maxCombo=0; jClient.position=0; jClient.linesSent=0; jClient.linesReceived=0;
		jClient.linesBlocked=0; jClient.bpm=0; jClient.spm=0; jClient.datavalid=false; jClient.score=0; jClient.incLines=0;
		jClient.away=false;
		jClient.datacount=250;
		jClient.ready=false;
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
			if ((gamemode == 1 || gamemode == 2) && !lClient.guest) {
				leavers.push_back(lClient);
				leavers.back().s_points=0;
			}
			it = clients.erase(it);
			if (activePlayers < 2)
				setInactive();
			lClient.incLines = 0;
			lClient.room = nullptr;
			std::cout << lClient.id << " left room " << id << std::endl;

			sendSignal(6, lClient.id);
			sendSignalToSpectators(6, lClient.id);

			if (gamemode == 4)
				if (tournamentGame != nullptr)
					tournamentGame->resetWaitTimeSent();
			break;
		}
}

bool Room::addSpectator(Client& client) {
	if (client.room || client.spectating == this)
		return false;
	if (client.spectating)
		client.spectating->removeSpectator(client);
	spectators.push_back(&client);
	client.spectating = this;
	return true;
}

void Room::removeSpectator(Client& client) {
	for (auto it = spectators.begin(); it != spectators.end(); it++)
		if ((*it)->id == client.id) {
			spectators.erase(it);
			client.spectating = nullptr;
			return;
		}
}

void Room::sendNewPlayerInfo() {
	conn->packet.clear();
	sf::Uint8 packetid = 4;
	conn->packet << packetid << conn->sender->id << conn->sender->name;
	sendPacket();
}

//Sorting function
bool sortClient(Client* client1, Client* client2) {
	return client1->score > client2->score;
}
void Room::sendRoundScores() {
	conn->packet.clear();
	sf::Uint8 packetid = 8, count=0;
	for (auto&& client : clients)
		if (client->position)
			count++;
	conn->packet << packetid << count;
	clients.sort(&sortClient);
	for (auto&& client : clients) {
		if (client->position) {
			conn->packet << client->id << client->maxCombo << client->linesSent << client->linesReceived;
			conn->packet << client->linesBlocked << client->bpm << client->spm << client->s_rank << client->position;
			conn->packet << client->score << client->linesAdjusted;
		}
	}
	sendPacket();
}

void Room::updatePlayerScore() {
	for (auto&& client : clients) {
		if (client->maxCombo > client->s_maxCombo)
			client->s_maxCombo = client->maxCombo;
		client->s_totalBpm+=client->bpm;
		client->s_avgBpm = (float)client->s_totalBpm / (float)client->s_gamesPlayed;
	}
}

void Room::scoreFFARound() {
	if (currentPlayers + leavers.size() < 2)
		return;

	short playersinround=0;
	float avgrank=0;
	for (auto&& client : clients) {
		if (client->position)
			client->score += currentPlayers - client->position;
		if (client->position && client->s_rank && !client->guest) {
			playersinround++;
			avgrank+=client->s_rank;
		}
	}
	for (auto&& client : leavers) {
		if (client.s_rank) {
			playersinround++;
			avgrank+=client.s_rank;
		}
	}
	avgrank/=playersinround; // Determine number of players to be scored & avg rank

	std::cout << "Scoring: " << playersinround << " players, avg rank: " << avgrank << std::endl;

	float* pointcoff = new float[playersinround];
	Client** inround = new Client*[playersinround];

	short lookingfor=0, position=1;
	while (lookingfor<playersinround) {
		for (auto&& client : clients)
			if (position == client->position && client->s_rank && !client->guest) {
				inround[lookingfor] = client;
				lookingfor++;
				break;
			}
		for (auto&& client : leavers)
			if (position == client.position && client.s_rank) {
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

void Room::scoreTournamentRound() {
	if (tournamentGame == nullptr)
		return;
	for (auto&& client : clients)
		if (client->position == 1) {
			if (tournamentGame->player1->id == client->id) {
				if (tournamentGame->p1won())
					active=false;
			}
			else if (tournamentGame->player2->id == client->id) {
				if (tournamentGame->p2won())
					active=false;
			}
			tournamentGame->sendScore();
			return;
		}
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

void Room::setInactive() {
	if (!active) return;
	active=false;
	if (round) {
		endround=true;
		checkIfRoundEnded();
	}
	else
		endRound();
	round=false;
	countdown=0;
	start.restart();
}

void Room::setActive() {
	if (gamemode == 4)
		if (tournamentGame != nullptr)
			if (tournamentGame->status == 4)
				return;
	active=true;
}

void Room::sendGameData() {
	for (auto&& fromClient : clients) {
		if (fromClient->datavalid) {
			for (auto&& toClient : clients)
				if (fromClient->id != toClient->id)
					conn->send(*fromClient, *toClient);
			for (auto&& toSpectator : spectators)
				conn->send(*fromClient, *toSpectator);
			fromClient->datavalid=false;
		}
	}
}

void Room::makeCountdown() {
	if (!round) {
		if (countdown) {
			if (start.getElapsedTime() > sf::seconds(1)) {
				countdown--;
				start.restart();
				sendSignalToActive(5, countdown);
				if (countdown == 0)
					startGame();
			}
		}
		else if (timeBetweenRounds != sf::seconds(0) && start.getElapsedTime() > timeBetweenRounds)
			startCountdown();
		else {
			bool allready=true;
			for (auto&& client : clients)
				if (!client->away && !client->ready)
					allready=false;
			if (allready && !waitForReplay)
				startCountdown();
		}
	}
}

void Room::checkIfRoundEnded() {
	if (round) {
		if (endround) {
			cout << "Round ended" << endl;
			for (auto&& winner : clients)
				if (winner->alive) {
					winner->position=1;
					winner->sendSignal(8);

					sendSignal(13, winner->id, winner->position);
					sendSignalToSpectators(13, winner->id, winner->position);
					break;
				}
			if (gamemode == 1)
				scoreFFARound();
			else if (gamemode == 4)
				scoreTournamentRound();
			endRound();
		}
	}
}

void Room::sendLines(Client& client) {
	if (playersAlive == 1)
		return;
	sf::Uint16 amount;
	conn->packet >> amount;
	float lineAdjust=0, sending=amount, sent=client.linesSent, actualSend;
	for (auto&& adj : adjust) {
		if (sent>=adj.amount)
			continue;
		if (adj.amount-sent<sending) {
			lineAdjust += (float)(adj.amount-sent) / (float)adj.players;
			sent += (float)(adj.amount-sent) / (float)adj.players;
			sending -= (float)(adj.amount-sent) / (float)adj.players;
		}
		else {
			lineAdjust += sending / (float)adj.players;
			sent += sending / (float)adj.players;
			sending -= sending / (float)adj.players;
		}
	}
	client.linesSent+=amount;
	client.linesAdjusted+=lineAdjust;
	actualSend=(float)amount-lineAdjust;
	actualSend/= ((float)playersAlive-1.0);
	std::cout << "amount: " << (int)amount << " adjust: " << lineAdjust << " players: " << (int)playersAlive << std::endl;
	std::cout << "Sending " << actualSend << " to room from " << client.id << std::endl;
	for (auto&& sendTo : clients)
		if (sendTo->id != client.id && sendTo->alive)
			sendTo->incLines+=actualSend;
}

void Room::sendSignal(sf::Uint8 signalId, int id1, int id2) {
	sf::Packet packet;
	packet << (sf::Uint8)254 << signalId;
	if (id1 > -1)
		packet << (sf::Uint16)id1;
	if (id2 > -1)
		packet << (sf::Uint16)id2;
	for (auto&& client : clients)
		if (client->socket.send(packet) != sf::Socket::Done)
			std::cout << "Error sending Signal packet to room " << id << std::endl;
}

void Room::sendSignalToAway(sf::Uint8 signalId, int id1, int id2) {
	sf::Packet packet;
	packet << (sf::Uint8)254 << signalId;
	if (id1 > -1)
		packet << (sf::Uint16)id1;
	if (id2 > -1)
		packet << (sf::Uint16)id2;
	for (auto&& client : clients)
		if (client->away)
			if (client->socket.send(packet) != sf::Socket::Done)
				std::cout << "Error sending Signal packet to room " << id << std::endl;
}

void Room::sendSignalToActive(sf::Uint8 signalId, int id1, int id2) {
	sf::Packet packet;
	packet << (sf::Uint8)254 << signalId;
	if (id1 > -1)
		packet << (sf::Uint16)id1;
	if (id2 > -1)
		packet << (sf::Uint16)id2;
	for (auto&& client : clients)
		if (!client->away)
			if (client->socket.send(packet) != sf::Socket::Done)
				std::cout << "Error sending Signal packet to room " << id << std::endl;
}


void Room::sendSignalToSpectators(sf::Uint8 signalId, int id1, int id2) {
	sf::Packet packet;
	packet << (sf::Uint8)254 << signalId;
	if (id1 > -1)
		packet << (sf::Uint16)id1;
	if (id2 > -1)
		packet << (sf::Uint16)id2;
	for (auto&& spectator : spectators)
		if (spectator->socket.send(packet) != sf::Socket::Done)
			std::cout << "Error sending Signal packet to spectator in room " << id << std::endl;
}

void Room::sendPacket() {
	sendPacketToPlayers();
	sendPacketToSpectators();
}

void Room::sendPacketToPlayers() {
	for (auto&& client : clients)
		if (client->socket.send(conn->packet) != sf::Socket::Done)
			std::cout << "Error sending packet to spectator in room " << id << std::endl;
}

void Room::sendPacketToSpectators() {
	for (auto&& spectator : spectators)
		if (spectator->socket.send(conn->packet) != sf::Socket::Done)
			std::cout << "Error sending packet to spectator in room " << id << std::endl;
}