#include "Room.h"
#include "Connections.h"
#include "Tournament.h"
#include <algorithm>
using std::cout;
using std::endl;

Room::Room(Connections& _conn, uint16_t _gamemode) :
conn(_conn),
active(false),
round(false),
waitForReplay(false),
locked(false),
gamemode(_gamemode),
tournamentGame(nullptr),
aiManager(start, botSendLines) {
	botSendLines.connect(&Room::sendLines, this);
}

void Room::startGame() {
	round=true;
	playersAlive = activePlayers;
	leavers.clear();
	lineSendAdjust.clear();
	endround=false;
	countdown=false;
	start.restart();
	for (auto&& client : clients) {
		client->datavalid=false;
		client->datacount=250;
		client->roundStats.clear();
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
				client.stats.updateFFARank();
				client.stats.vsPoints = leaver.stats.vsPoints;
				client.stats.heroPoints = leaver.stats.heroPoints;
			}
		for (auto&& client : conn.uploadData)
			if (leaver.id == client.id) {
				client.stats.ffaPoints += leaver.stats.ffaPoints;
				client.stats.updateFFARank();
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
		jClient.alive=false;
		jClient.datavalid=false;
		jClient.away=false;
		jClient.datacount=250;
		jClient.ready=false;
		jClient.roundStats.clear();
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
				lClient.roundStats.position=playersAlive;
				playerDied(lClient);
			}
			if ((gamemode == 1 || gamemode == 2 || gamemode == 4 || gamemode || 5) && !lClient.guest) {
				leavers.push_back(lClient);
				leavers.back().stats.ffaPoints=0;
			}
			it = clients.erase(it);
			if (activePlayers < 2)
				setInactive();
			lClient.roundStats.incLines = 0;
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
		if (clients.front()->roundStats.score == 4 || clients.back()->roundStats.score == 4) {
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
	sf::Packet packet;
	uint8_t packetid = 4;
	packet << packetid << client.id << client.name;
	sendPacket(packet);
}

void Room::sendRoundScores() {
	sf::Packet packet;
	uint8_t packetid = 8, count=0;
	for (auto&& client : clients)
		if (client->roundStats.position)
			count++;
	packet << packetid << (uint16_t)roundLenght.asSeconds() << count;
	clients.sort([](Client* c1, Client* c2) { return c1->roundStats.score > c2->roundStats.score; });
	for (auto&& client : clients) {
		if (client->roundStats.position) {
			packet << client->id << client->roundStats.maxCombo << client->roundStats.linesSent << client->roundStats.linesReceived;
			packet << client->roundStats.linesBlocked << client->roundStats.bpm << client->stats.ffaRank << client->roundStats.position;
			packet << client->roundStats.score << client->roundStats.linesAdjusted << uint16_t(client->stats.ffaPoints+1000);
		}
	}
	sendPacket(packet);
}

void Room::updatePlayerScore() {
	for (auto&& client : clients) {
		if (client->roundStats.maxCombo > client->stats.maxCombo)
			client->stats.maxCombo = client->roundStats.maxCombo;
		client->stats.totalBpm+=client->roundStats.bpm;
		if (client->stats.totalPlayed > 0)
			client->stats.avgBpm = (float)client->stats.totalBpm / (float)client->stats.totalPlayed;
	}
}

void Room::score1vs1Round() {
	if (currentPlayers + leavers.size() < 2)
		return;

	Client *winner = nullptr, *loser = nullptr;
	for (auto client : clients)
		if (client->roundStats.position == 1)
			winner = client;
	for (auto client : clients)
		if (client->roundStats.position == 2)
			loser = client;
	for (auto client : leavers)
		if (client.roundStats.position == 2)
			loser = &client;

	if (!winner || !loser)
		return;

	winner->roundStats.score++;
	eloResults.addResult(*winner, *loser, 1);
	eloResults.calculateResults();

	if (gamemode == 4) {
		tournamentGame->sendScore();
		return;
	}

	sf::Packet packet;
	packet << (uint8_t)24 << winner->id << loser->id;
	if (winner->roundStats.score == 4 || loser->roundStats.score == 4) {
		lock();
		packet << (uint8_t)255;
	}
	else
		packet << (uint8_t)0;
	packet << (uint8_t)0 << (uint8_t)winner->roundStats.score << (uint8_t)loser->roundStats.score;
	
	sendPacket(packet);

	timeBetweenRounds = sf::seconds(3);
}

void Room::playerDied(Client& died) {
	playersAlive--;
	lineSendAdjust.addAdjust(died, clients, playersAlive);
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
						uint16_t compensate = std::min(client->ping.getAverage()/2, client->ping.getLowest());
						compensate=3000-t.asMilliseconds()-compensate;
						sf::Packet packet;
						packet << (uint8_t)103 << compensate;
						conn.sendUDP(*client, packet);
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
					winner->roundStats.position=1;
					winner->sendSignal(8);

					sendSignal(13, winner->id, winner->roundStats.position);
					sendSignalToSpectators(13, winner->id, winner->roundStats.position);

					scoreRound();
					transfearScore();
					break;
				}
			endRound();
		}
	}
}

void Room::sendLines(uint16_t id, RoundStats& stats, uint16_t amount) {
	if (playersAlive == 1)
		return;

	float lineAdjust = lineSendAdjust.getAdjust(stats.linesSent, amount);

	stats.linesSent+=amount;
	stats.linesAdjusted+=lineAdjust;
	float actualSend=amount-lineAdjust;
	actualSend /= playersAlive-1;

	distributeLines(id, actualSend);
}

void Room::distributeLines(uint16_t senderid, float amount) {
	for (auto& client : clients)
		if (client->id != senderid && client->alive)
			client->roundStats.incLines+=amount;

	aiManager.sendLines(senderid, amount);
}

void Room::sendSignal(uint8_t signalId, int id1, int id2) {
	sf::Packet packet;
	packet << (uint8_t)254 << signalId;
	if (id1 > -1)
		packet << (uint16_t)id1;
	if (id2 > -1)
		packet << (uint16_t)id2;
	for (auto&& client : clients)
		if (client->socket->send(packet) != sf::Socket::Done)
			std::cout << "Error sending Signal packet to room " << id << std::endl;
}

void Room::sendSignalToAway(uint8_t signalId, int id1, int id2) {
	sf::Packet packet;
	packet << (uint8_t)254 << signalId;
	if (id1 > -1)
		packet << (uint16_t)id1;
	if (id2 > -1)
		packet << (uint16_t)id2;
	for (auto&& client : clients)
		if (client->away)
			if (client->socket->send(packet) != sf::Socket::Done)
				std::cout << "Error sending Signal packet to room " << id << std::endl;
}

void Room::sendSignalToActive(uint8_t signalId, int id1, int id2) {
	sf::Packet packet;
	packet << (uint8_t)254 << signalId;
	if (id1 > -1)
		packet << (uint16_t)id1;
	if (id2 > -1)
		packet << (uint16_t)id2;
	for (auto&& client : clients)
		if (!client->away)
			if (client->socket->send(packet) != sf::Socket::Done)
				std::cout << "Error sending Signal packet to room " << id << std::endl;
}


void Room::sendSignalToSpectators(uint8_t signalId, int id1, int id2) {
	sf::Packet packet;
	packet << (uint8_t)254 << signalId;
	if (id1 > -1)
		packet << (uint16_t)id1;
	if (id2 > -1)
		packet << (uint16_t)id2;
	for (auto&& spectator : spectators)
		if (spectator->socket->send(packet) != sf::Socket::Done)
			std::cout << "Error sending Signal packet to spectator in room " << id << std::endl;
}

void Room::sendPacket(sf::Packet& packet) {
	sendPacketToPlayers(packet);
	sendPacketToSpectators(packet);
}

void Room::sendPacketToPlayers(sf::Packet& packet) {
	for (auto& client : clients)
		if (client->socket->send(packet) != sf::Socket::Done)
			std::cout << "Error sending packet to player in room " << id << std::endl;
}

void Room::sendPacketToSpectators(sf::Packet& packet) {
	for (auto& spectator : spectators)
		if (spectator->socket->send(packet) != sf::Socket::Done)
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
		if (client->roundStats.position) {
			client->roundStats.score += currentPlayers - client->roundStats.position;
			if (client->stats.ffaRank && !client->guest) {
				inround.push_back(client);
				avgrank+=client->stats.ffaRank;
			}
		}
	}
	for (auto& client : leavers) {
		if (client.roundStats.position && client.stats.ffaRank) {
			inround.push_back(&client);
			avgrank+=client.stats.ffaRank;
		}
	}
	float playersinround = inround.size();
	if (!playersinround)
		return;
	avgrank/=playersinround;

	std::sort(inround.begin(), inround.end(), [](Client* c1, Client* c2) { return c1->roundStats.position < c2->roundStats.position; });

	int i=0;
	for (auto client : inround) {
		float pointcoff = ((((float)i+1)/(float)playersinround) - 1.0/(float)playersinround  - (1.0-1.0/(float)playersinround)/2.0) * (-1.0/ ((1.0-1.0/(float)playersinround)/2.0) );
		pointcoff += (client->stats.ffaRank - avgrank) * 0.05 + 0.2;
		client->stats.ffaPoints += 100*pointcoff*(client->stats.ffaRank/5.0);
		client->stats.updateFFARank();

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
		if (winner->roundStats.position) {
			for (auto loser : clients)
				if (loser->roundStats.position && winner->roundStats.position < loser->roundStats.position)
					eloResults.addResult(*winner, *loser, 2);
			for (auto loser : leavers)
				if (loser.roundStats.position && winner->roundStats.position < loser.roundStats.position)
					eloResults.addResult(*winner, loser, 2);
		}
	for (auto winner : leavers)
		if (winner.roundStats.position) {
			for (auto loser : clients)
				if (loser->roundStats.position && winner.roundStats.position < loser->roundStats.position)
					eloResults.addResult(winner, *loser, 2);
			for (auto loser : leavers)
				if (loser.roundStats.position && winner.roundStats.position < loser.roundStats.position)
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
		if (client->roundStats.position == 1) {
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