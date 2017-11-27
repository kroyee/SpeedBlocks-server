#include "Room.h"
#include "Connections.h"
#include "Tournament.h"
#include <algorithm>
using std::cout;
using std::endl;

Room::Room(Connections& _conn, sf::Uint16 _gamemode) :
conn(_conn),
active(false),
round(false),
waitForReplay(false),
locked(false),
gamemode(_gamemode),
tournamentGame(nullptr) {}

void Room::startGame() {
	round=true;
	playersAlive = activePlayers;
	leavers.clear();
	adjust.clear();
	endround=false;
	countdown=false;
	start.restart();
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
			incrementGamesPlayed(*client);
			client->alive=true;
		}
	}
}

void Room::startCountdown() {
	countdownTime=sf::seconds(0);
	countdown=true;
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
		for (auto&& client : conn.clients)
			if (leaver.id == client.id) {
				client.stats.ffaPoints += leaver.stats.ffaPoints;
				if (client.stats.ffaPoints>1000) {
					client.stats.ffaPoints=0;
					client.stats.ffaRank--;
				}
				if (client.stats.ffaPoints<-1000) {
					client.stats.ffaPoints=0;
					client.stats.ffaRank++;
				}
				client.stats.vsPoints = leaver.stats.vsPoints;
				client.stats.heroPoints = leaver.stats.heroPoints;
			}
		for (auto&& client : conn.uploadData)
			if (leaver.id == client.id) {
				client.stats.ffaPoints += leaver.stats.ffaPoints;
				if (client.stats.ffaPoints>1000) {
					client.stats.ffaPoints=0;
					client.stats.ffaRank--;
				}
				if (client.stats.ffaPoints<-1000) {
					client.stats.ffaPoints=0;
					client.stats.ffaRank++;
				}
				client.stats.vsPoints = leaver.stats.vsPoints;
				client.stats.heroPoints = leaver.stats.heroPoints;
			}
	}
	leavers.clear();
}

void Room::endRound() {
	if (round)
		for (auto&& client : clients)
			client->ready=false;
	round=false;
	countdown=false;
	roundLenght = start.restart();
	sendSignal(7);
}

void Room::join(Client& jClient) {
	if (currentPlayers<maxPlayers || maxPlayers == 0) {
		if (jClient.spectating)
			jClient.spectating->removeSpectator(jClient);
		if (jClient.matchmaking && gamemode != 5) {
			conn.lobby.matchmaking1vs1.removeFromQueue(jClient);
			jClient.sendSignal(21);
		}
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
		jClient.history.clear();
		std::cout << jClient.id << " joined room " << id << std::endl;
	}
}

void Room::leave(Client& lClient) {
	for (auto it = clients.begin(); it != clients.end(); it++)
		if ((*it)->id == lClient.id) {
			matchLeaver(lClient);
			currentPlayers--;
			if (!lClient.away)
				activePlayers--;
			if (lClient.alive) {
				lClient.position=playersAlive;
				playerDied(lClient);
			}
			if ((gamemode == 1 || gamemode == 2 || gamemode == 4 || gamemode || 5) && !lClient.guest) {
				leavers.push_back(lClient);
				leavers.back().stats.ffaPoints=0;
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

void Room::matchLeaver(Client& lClient) {
	if (gamemode != 5)
		return;
	if (currentPlayers == 2) {
		if (clients.front()->score == 4 || clients.back()->score == 4) {
			conn.lobby.matchmaking1vs1.setQueueing(lClient, conn.serverClock.getElapsedTime());
			lClient.sendSignal(22);
		}
		else {
			conn.lobby.matchmaking1vs1.removeFromQueue(lClient);
			lClient.sendSignal(21);
		}
	}
	else {
		conn.lobby.matchmaking1vs1.setQueueing(lClient, conn.serverClock.getElapsedTime());
		lClient.sendSignal(22);
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

void Room::sendNewPlayerInfo(Client& client) {
	conn.packet.clear();
	sf::Uint8 packetid = 4;
	conn.packet << packetid << client.id << client.name;
	sendPacket();
}

void Room::sendRoundScores() {
	conn.packet.clear();
	sf::Uint8 packetid = 8, count=0;
	for (auto&& client : clients)
		if (client->position)
			count++;
	conn.packet << packetid << (sf::Uint16)roundLenght.asSeconds() << count;
	clients.sort([](Client* c1, Client* c2) { return c1->score > c2->score; });
	for (auto&& client : clients) {
		if (client->position) {
			conn.packet << client->id << client->maxCombo << client->linesSent << client->linesReceived;
			conn.packet << client->linesBlocked << client->bpm << client->stats.ffaRank << client->position;
			conn.packet << client->score << client->linesAdjusted << sf::Uint16(client->stats.ffaPoints+1000);
		}
	}
	sendPacket();
}

void Room::updatePlayerScore() {
	for (auto&& client : clients) {
		if (client->maxCombo > client->stats.maxCombo)
			client->stats.maxCombo = client->maxCombo;
		client->stats.totalBpm+=client->bpm;
		if (client->stats.totalPlayed > 0)
			client->stats.avgBpm = (float)client->stats.totalBpm / (float)client->stats.totalPlayed;
	}
}

void Room::score1vs1Round() {
	if (currentPlayers + leavers.size() < 2)
		return;

	Client *winner = nullptr, *loser = nullptr;
	for (auto client : clients)
		if (client->position == 1)
			winner = client;
	for (auto client : clients)
		if (client->position == 2)
			loser = client;
	for (auto client : leavers)
		if (client.position == 2)
			loser = &client;

	if (!winner || !loser)
		return;

	winner->score++;
	eloResults.addResult(*winner, *loser, 1);
	eloResults.calculateResults();

	if (gamemode == 4) {
		tournamentGame->sendScore();
		return;
	}

	conn.packet.clear();
	conn.packet << (sf::Uint8)24 << winner->id << loser->id;
	if (winner->score == 4 || loser->score == 4) {
		lock();
		conn.packet << (sf::Uint8)255;
	}
	else
		conn.packet << (sf::Uint8)0;
	conn.packet << (sf::Uint8)0 << (sf::Uint8)winner->score << (sf::Uint8)loser->score;
	
	sendPacket();

	timeBetweenRounds = sf::seconds(3);
}

void Room::playerDied(Client& died) {
	int second=0;
	bool leaderDied=false;
	Adjust adj;
	adj.amount=0;
	for (auto&& client : clients) {
		if (client->linesSent > adj.amount) {
			second = adj.amount;
			adj.amount = client->linesSent;
			if (died.id == client->id)
				leaderDied=true;
			else
				leaderDied=false;
		}
		else if (client->linesSent > second)
			second = client->linesSent;
	}
	if (leaderDied)
		adj.amount = second;
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
	countdown=false;
	start.restart();
}

void Room::setActive() {
	if (locked)
		return;
	active=true;
	countdown=false;
}

void Room::lock() {
	active=false;
	locked=true;
}

void Room::sendGameData() {
	for (auto&& fromClient : clients) {
		if (fromClient->datavalid) {
			for (auto&& toClient : clients)
				if (fromClient->id != toClient->id)
					conn.send(*fromClient, *toClient);
			for (auto&& toSpectator : spectators)
				conn.send(*fromClient, *toSpectator);
			fromClient->datavalid=false;
		}
	}
}

void Room::makeCountdown() {
	if (!round) {
		if (countdown) {
			sf::Time t = start.getElapsedTime();
			if (t > sf::seconds(3)) {
				start.restart();
				sendSignalToActive(5, 1);
				sendSignalToAway(5, 0);
				sendSignalToSpectators(5, 0);
				startGame();
			}
			else if (t > countdownTime) {
				countdownTime+=sf::milliseconds(200);
				for (auto& client : clients) {
					if (!client->away) {
						sf::Uint16 compensate = std::min(client->ping.getAverage()/2, client->ping.getLowest());
						compensate=3000-t.asMilliseconds()-compensate;
						conn.packet.clear();
						conn.packet << (sf::Uint8)103 << compensate;
						conn.sendUDP(*client);
					}
				}
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
			for (auto&& winner : clients)
				if (winner->alive) {
					winner->alive=false;
					winner->position=1;
					winner->sendSignal(8);

					sendSignal(13, winner->id, winner->position);
					sendSignalToSpectators(13, winner->id, winner->position);

					scoreRound();
					transfearScore();
					break;
				}
			endRound();
		}
	}
}

void Room::sendLines(Client& client) {
	if (playersAlive == 1)
		return;
	sf::Uint16 amount;
	conn.packet >> amount;
	float lineAdjust=0, sending=amount, sent=client.linesSent, actualSend;
	for (auto&& adj : adjust) {
		if (sent>=adj.amount)
			continue;
		else if (adj.players == 0)
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
	if (playersAlive-1 != 0)
		actualSend/= ((float)playersAlive-1.0);
	else
		actualSend=0;
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
		if (client->socket->send(packet) != sf::Socket::Done)
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
			if (client->socket->send(packet) != sf::Socket::Done)
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
			if (client->socket->send(packet) != sf::Socket::Done)
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
		if (spectator->socket->send(packet) != sf::Socket::Done)
			std::cout << "Error sending Signal packet to spectator in room " << id << std::endl;
}

void Room::sendPacket() {
	sendPacketToPlayers();
	sendPacketToSpectators();
}

void Room::sendPacketToPlayers() {
	for (auto&& client : clients)
		if (client->socket->send(conn.packet) != sf::Socket::Done)
			std::cout << "Error sending packet to player in room " << id << std::endl;
}

void Room::sendPacketToSpectators() {
	for (auto&& spectator : spectators)
		if (spectator->socket->send(conn.packet) != sf::Socket::Done)
			std::cout << "Error sending packet to spectator in room " << id << std::endl;
}

//////////////////////////////////////////////
//					FFA						//
//////////////////////////////////////////////

void FFARoom::scoreRound() {
	if (currentPlayers + leavers.size() < 2)
		return;

	std::vector<Client*> inround;
	float avgrank=0;

	for (auto& client : clients) {
		if (client->position) {
			client->score += currentPlayers - client->position;
			if (client->stats.ffaRank && !client->guest) {
				inround.push_back(client);
				avgrank+=client->stats.ffaRank;
			}
		}
	}
	for (auto& client : leavers) {
		if (client.position && client.stats.ffaRank) {
			inround.push_back(&client);
			avgrank+=client.stats.ffaRank;
		}
	}
	float playersinround = inround.size();
	if (!playersinround)
		return;
	avgrank/=playersinround;

	std::sort(inround.begin(), inround.end(), [](Client* c1, Client* c2) { return c1->position < c2->position; });

	int i=0;
	for (auto client : inround) {
		float pointcoff = ((((float)i+1)/(float)playersinround) - 1.0/(float)playersinround  - (1.0-1.0/(float)playersinround)/2.0) * (-1.0/ ((1.0-1.0/(float)playersinround)/2.0) );
		pointcoff += (client->stats.ffaRank - avgrank) * 0.05 + 0.2;
		client->stats.ffaPoints += 100*pointcoff*(client->stats.ffaRank/5.0);
		if (client->stats.ffaPoints > 1000) {
			client->stats.ffaRank--;
			client->stats.ffaPoints=0;
		}
		else if (client->stats.ffaPoints < -1000) {
			if (client->stats.ffaRank == 25)
				client->stats.ffaPoints = -1000;
			else {
				client->stats.ffaRank++;
				client->stats.ffaPoints=0;
			}
		}

		i++;
	}
}

//////////////////////////////////////////////
//					Hero					//
//////////////////////////////////////////////

void HeroRoom::scoreRound() {
	if (currentPlayers + leavers.size() < 2)
		return;

	for (auto winner : clients)
		if (winner->position) {
			for (auto loser : clients)
				if (loser->position && winner->position < loser->position)
					eloResults.addResult(*winner, *loser, 2);
			for (auto loser : leavers)
				if (loser.position && winner->position < loser.position)
					eloResults.addResult(*winner, loser, 2);
		}
	for (auto winner : leavers)
		if (winner.position) {
			for (auto loser : clients)
				if (loser->position && winner.position < loser->position)
					eloResults.addResult(winner, *loser, 2);
			for (auto loser : leavers)
				if (loser.position && winner.position < loser.position)
					eloResults.addResult(winner, loser, 2);
		}

	eloResults.calculateResults();
}

//////////////////////////////////////////////
//					1vs1					//
//////////////////////////////////////////////

void VSRoom::scoreRound() {
	score1vs1Round();
}

//////////////////////////////////////////////
//					Casual					//
//////////////////////////////////////////////

//////////////////////////////////////////////
//					Tournament				//
//////////////////////////////////////////////

void TournamentRoom::scoreRound() {
	if (tournamentGame == nullptr)
		return;
	for (auto&& client : clients)
		if (client->position == 1) {
			if (tournamentGame->player1->id == client->id) {
				if (tournamentGame->p1won())
					lock();
			}
			else if (tournamentGame->player2->id == client->id) {
				if (tournamentGame->p2won())
					lock();
			}
			score1vs1Round();
			return;
		}
}

//////////////////////////////////////////////
//					Challenge				//
//////////////////////////////////////////////