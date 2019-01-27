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
tournamentGame(nullptr) {}

void Room::startGame() {
	round=true;
	playersAlive = activePlayers;
	leavers.clear();
	lineSendAdjust.clear();
	endround=false;
	countdown=false;
	start.restart();

	for (auto&& client : clients)
		client->startGame();
}

void Room::startCountdown() {
	countdownTime=sf::seconds(0);
	countdown=true;
	playersAlive=activePlayers;
	start.restart();
	seed1 = rand();
	seed2 = rand();

	for (auto& client : clients)
		client->updateSpeed();

	for (auto& client : clients)
		client->seed(seed1, seed2);

	for (auto& client : spectators)
		client->seed(seed1, seed2, 10);
}

void Room::transfearScore() {
	for (auto&& leaver : leavers) {
		for (auto&& client : conn.clients)
			if (leaver.id == client->id) {
				client->stats.ffaPoints += leaver.stats.ffaPoints;
				client->stats.updateFFARank();
				client->stats.vsPoints = leaver.stats.vsPoints;
				client->stats.heroPoints = leaver.stats.heroPoints;
			}
		for (auto&& client : conn.uploadData)
			if (leaver.id == client->id) {
				client->stats.ffaPoints += leaver.stats.ffaPoints;
				client->stats.updateFFARank();
				client->stats.vsPoints = leaver.stats.vsPoints;
				client->stats.heroPoints = leaver.stats.heroPoints;
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

	for (auto& client : clients)
		client->endRound();
}

void Room::join(Client& client) {
	if (currentPlayers<maxPlayers || maxPlayers == 0) {
		if (client.spectating)
			client.spectating->removeSpectator(client);
		if (client.matchmaking && gamemode != 5) {
			conn.lobby.matchmaking1vs1.removeFromQueue(client);
			client.sendSignal(21);
		}
		currentPlayers++;
		activePlayers++;
		clients.push_back(&client);

		if ((activePlayers > 1 && !onlyBots()) || gamemode >= 20000)
			setActive();

		client.clear();
		client.room = this;
		std::cout << client.name << " (" << client.id << ") joined room " << id << std::endl;
	}
}

void Room::leave(Client& client) {
	for (auto it = clients.begin(); it != clients.end(); it++)
		if ((*it)->id == client.id) {
			matchLeaver(client);
			currentPlayers--;
			if (!client.away)
				activePlayers--;
			if (client.alive) {
				client.roundStats.position=playersAlive;
				playerDied(client);
			}
			if ((gamemode == 1 || gamemode == 2 || gamemode == 4 || gamemode || 5) && !client.guest) {
				leavers.emplace_back();
				leavers.back().roundStats = client.roundStats;
				leavers.back().stats = client.stats;
				leavers.back().id = client.id;
				leavers.back().stats.ffaPoints=0;
			}
			it = clients.erase(it);
			if (activePlayers < 2 || onlyBots())
				setInactive();
			client.roundStats.incLines = 0;
			client.room = nullptr;
			std::cout << client.name << " (" << client.id << ") left room " << id << std::endl;

			sendSignal(6, client.id);
			sendSignalToSpectators(6, client.id);

			if (gamemode == 4)
				if (tournamentGame != nullptr)
					tournamentGame->resetWaitTimeSent();
			break;
		}
}

void Room::matchLeaver(Client& client) {
	if (gamemode != 5)
		return;
	if (currentPlayers == 2) {
		if (clients.front()->roundStats.score == 4 || clients.back()->roundStats.score == 4) {
			conn.lobby.matchmaking1vs1.setQueueing(client, conn.serverClock.getElapsedTime());
			client.sendSignal(22);
		}
		else {
			conn.lobby.matchmaking1vs1.removeFromQueue(client);
			client.sendSignal(21);
		}
	}
	else {
		conn.lobby.matchmaking1vs1.setQueueing(client, conn.serverClock.getElapsedTime());
		client.sendSignal(22);
	}
}

bool Room::addSpectator(HumanClient& client) {
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
	for (auto& client : leavers)
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

void Room::sendGameData(sf::UdpSocket& udp) {
	for (auto&& client : clients) {
		if (client->datavalid)
			client->sendGameData(udp);

		sendLines(*client, client->sendLinesOut());
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
						client->countDown(t);
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

					if (!winner->isHuman())
						winner->makeWinner();
					break;
				}
			endRound();
		}
	}
}

void Room::sendLines(Client& client, uint16_t amount) {
	uint16_t amountbefore = amount;
	amount = client.hcp.send(amount);
	if (amountbefore)
		cout << client.id << " sending " << amountbefore << " -> " << amount << endl;
	if (playersAlive == 1 || !amount)
		return;

	float lineAdjust = lineSendAdjust.getAdjust(client.roundStats.linesSent, amount);

	client.roundStats.linesSent+=amount;
	client.roundStats.linesAdjusted+=lineAdjust;
	float actualSend=amount-lineAdjust;
	actualSend /= playersAlive-1;

	distributeLines(client.id, actualSend);
}

void Room::distributeLines(uint16_t senderid, float amount) {
	for (auto& client : clients)
		if (client->id != senderid && client->alive) {
			client->roundStats.incLines+=amount;
			client->sendLines();
		}
}

void Room::sendSignal(uint8_t signalId, int id1, int id2) {
	sf::Packet packet;
	packet << (uint8_t)254 << signalId;
	if (id1 > -1)
		packet << (uint16_t)id1;
	if (id2 > -1)
		packet << (uint16_t)id2;
	for (auto&& client : clients)
		client->sendPacket(packet);
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
			client->sendPacket(packet);
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
			client->sendPacket(packet);
}


void Room::sendSignalToSpectators(uint8_t signalId, int id1, int id2) {
	sf::Packet packet;
	packet << (uint8_t)254 << signalId;
	if (id1 > -1)
		packet << (uint16_t)id1;
	if (id2 > -1)
		packet << (uint16_t)id2;
	for (auto&& spectator : spectators)
		spectator->sendPacket(packet);
}

void Room::sendPacket(sf::Packet& packet) {
	sendPacketToPlayers(packet);
	sendPacketToSpectators(packet);
}

void Room::sendPacketToPlayers(sf::Packet& packet) {
	for (auto& client : clients)
		client->sendPacket(packet);
}

void Room::sendPacketToSpectators(sf::Packet& packet) {
	for (auto& spectator : spectators)
		spectator->sendPacket(packet);
}

bool Room::onlyBots() {
	for (auto& client : clients)
		if (client->isHuman())
			return false;

	return spectators.empty();
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
			for (auto& loser : leavers)
				if (loser.roundStats.position && winner->roundStats.position < loser.roundStats.position)
					eloResults.addResult(*winner, loser, 2);
		}
	for (auto& winner : leavers)
		if (winner.roundStats.position) {
			for (auto loser : clients)
				if (loser->roundStats.position && winner.roundStats.position < loser->roundStats.position)
					eloResults.addResult(winner, *loser, 2);
			for (auto& loser : leavers)
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
