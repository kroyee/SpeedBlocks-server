#include "Room.h"
#include "Connections.h"

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
		setActive();
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
			if ((gamemode == 1 || gamemode == 2) && !lClient.guest) {
				leavers.push_back(lClient);
				leavers.back().s_points=0;
			}
			it = clients.erase(it);
			if (activePlayers == 0)
				setInactive();
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
	active=false;
	round=false;
	countdown=0;
	start.restart();
}

void Room::setActive() {
	active=true;
}

void Room::sendGameData() {
	for (auto&& fromClient : clients) {
		if (fromClient->datavalid) {
			for (auto&& toClient : clients)
				if (fromClient->id != toClient->id)
					conn->send(*fromClient, *toClient);
			fromClient->datavalid=false;
		}
	}
}

void Room::makeCountdown() {
	if (!round) {
		if (countdown) {
			if (start.getElapsedTime() > sf::seconds(1)) { //2-Packet 
				countdown--;
				start.restart();
				conn->packet.clear();
				sf::Uint8 packetid = 2;
				conn->packet << packetid;
				packetid = countdown;
				conn->packet << packetid;
				conn->send(*this, 1);
				if (countdown == 0)
					startGame();
			}
		}
		else if (start.getElapsedTime() > sf::seconds(3)) { //1-Packet
			countdown=countdownSetting;
			start.restart();
			conn->packet.clear();
			sf::Uint8 packetid = 1;
			conn->packet << packetid;
			packetid = countdown;
			conn->packet << packetid;
			seed1 = rand();
			seed2 = rand();
			conn->packet << seed1 << seed2;
			conn->send(*this, 1);
			conn->packet.clear(); //11-Packet
			packetid = 11;
			conn->packet << packetid << seed1 << seed2;
			conn->send(*this, 2);
		}
	}
}

void Room::checkIfRoundEnded() {
	if (round) {
		if (endround) {
			bool nowinner=true;
			conn->packet.clear();
			sf::Uint8 packetid = 7; // 7-Packet
			conn->packet << packetid;
			for (auto&& winner : clients)
				if (winner->alive) {
					winner->position=1;
					conn->send(*winner);
					nowinner=false;

					conn->packet.clear(); // 15-Packet
					packetid = 15;
					conn->packet << packetid << winner->id << winner->position;
					conn->send(*this);
					break;
				}
			if (nowinner) {
				conn->packet.clear();
				packetid = 7;
				conn->packet << packetid;
				conn->send(*this);
			}
			if (gamemode == 1)
				scoreFFARound();
			endRound();
		}
	}
}