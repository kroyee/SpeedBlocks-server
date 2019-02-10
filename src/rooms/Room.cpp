#include "Room.h"
#include <algorithm>
#include "Connections.h"
#include "Tournament.h"
using std::cout;
using std::endl;

Room::Room(Connections& _conn, uint16_t _gamemode)
    : conn(_conn), active(false), round(false), waitForReplay(false), locked(false), gamemode(_gamemode), tournamentGame(nullptr) {}

void Room::startGame() {
    round = true;
    playersAlive = activePlayers;
    leavers.clear();
    lineSendAdjust.clear();
    endround = false;
    countdown = false;
    start.restart();

    for (auto&& client : clients) client->startGame();
}

void Room::startCountdown() {
    countdownTime = sf::seconds(0);
    countdown = true;
    playersAlive = activePlayers;
    start.restart();
    seed1 = rand();
    seed2 = rand();

    for (auto& client : clients) client->updateSpeed();

    for (auto& client : clients) client->seed(seed1, seed2);

    for (auto& client : spectators) client->seed(seed1, seed2, 1);
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
        for (auto&& client : clients) client->ready = false;
    round = false;
    countdown = false;
    roundLenght = start.restart();

    for (auto& client : clients) client->endRound();
}

void Room::join(Client& jClient) {
    if (currentPlayers < maxPlayers || maxPlayers == 0) {
        if (jClient.spectating) jClient.spectating->removeSpectator(jClient);
        if (jClient.matchmaking && gamemode != 5) {
            conn.lobby.matchmaking1vs1.removeFromQueue(jClient);
            jClient.sendPacket(PM::make<NP_ClientRemovedMatchmaking>());
        }
        currentPlayers++;
        activePlayers++;
        clients.push_back(&jClient);
        if ((activePlayers > 1 && !onlyBots()) || gamemode >= 20000) setActive();
        jClient.room = this;
        jClient.alive = false;
        jClient.datavalid = false;
        jClient.away = false;
        jClient.data.count = 250;
        jClient.ready = false;
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
            if (!lClient.away) activePlayers--;
            if (lClient.alive) {
                lClient.roundStats.position = playersAlive;
                playerDied(lClient);
            }
            if ((gamemode == 1 || gamemode == 2 || gamemode == 4 || gamemode || 5) && !lClient.guest) {
                leavers.emplace_back();
                leavers.back().roundStats = lClient.roundStats;
                leavers.back().stats = lClient.stats;
                leavers.back().id = lClient.id;
                leavers.back().stats.ffaPoints = 0;
            }
            it = clients.erase(it);
            if (activePlayers < 2 || onlyBots()) setInactive();
            lClient.roundStats.incLines = 0;
            lClient.room = nullptr;
            std::cout << lClient.id << " left room " << id << std::endl;

            auto packet = PM::make<NP_ClientLeftRoom>(lClient.id);
            sendPacket(packet);
            sendPacketToSpectators(packet);

            if (gamemode == 4)
                if (tournamentGame != nullptr) tournamentGame->resetWaitTimeSent();
            break;
        }
}

void Room::matchLeaver(Client& lClient) {
    if (gamemode != 5) return;
    if (currentPlayers == 2) {
        if (clients.front()->roundStats.game_score == 4 || clients.back()->roundStats.game_score == 4) {
            conn.lobby.matchmaking1vs1.setQueueing(lClient, conn.serverClock.getElapsedTime());
            lClient.sendPacket(PM::make<NP_ClientStillInMatchmaking>());
        } else {
            conn.lobby.matchmaking1vs1.removeFromQueue(lClient);
            lClient.sendPacket(PM::make<NP_ClientRemovedMatchmaking>());
        }
    } else {
        conn.lobby.matchmaking1vs1.setQueueing(lClient, conn.serverClock.getElapsedTime());
        lClient.sendPacket(PM::make<NP_ClientStillInMatchmaking>());
    }
}

bool Room::addSpectator(HumanClient& client) {
    if (client.room || client.spectating == this) return false;
    if (client.spectating) client.spectating->removeSpectator(client);
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
    PM packet;
    NP_ClientJoinedRoom info{{client.id, client.name}};
    packet.write(info);
    sendPacket(packet);
}

void Room::sendRoundScores() {
    clients.sort([](Client* c1, Client* c2) { return c1->roundStats.game_score > c2->roundStats.game_score; });
    NP_RoomScore data;
    data.round_lenght = roundLenght.asSeconds();

    for (auto&& client : clients) {
        if (client->roundStats.position) {
            data.scores.push_back({client->roundStats.score, client->id, client->stats.ffaRank, client->roundStats.position, client->roundStats.game_score,
                                   client->roundStats.linesAdjusted, static_cast<uint16_t>(client->stats.ffaPoints + 1000)});
        }
    }

    PM packet;
    packet.write(data);
    sendPacket(packet);
}

void Room::updatePlayerScore() {
    for (auto&& client : clients) {
        if (client->roundStats.score.max_combo > client->stats.maxCombo) client->stats.maxCombo = client->roundStats.score.max_combo;
        client->stats.totalBpm += client->roundStats.score.bpm;
        if (client->stats.totalPlayed > 0) client->stats.avgBpm = (float)client->stats.totalBpm / (float)client->stats.totalPlayed;
    }
}

void Room::score1vs1Round() {
    if (currentPlayers + leavers.size() < 2) return;

    Client *winner = nullptr, *loser = nullptr;
    for (auto client : clients)
        if (client->roundStats.position == 1) winner = client;
    for (auto client : clients)
        if (client->roundStats.position == 2) loser = client;
    for (auto& client : leavers)
        if (client.roundStats.position == 2) loser = &client;

    if (!winner || !loser) return;

    winner->roundStats.game_score++;
    eloResults.addResult(*winner, *loser, 1);
    eloResults.calculateResults();

    if (gamemode == 4) {
        tournamentGame->sendScore();
        return;
    }

    const uint16_t game_limit_1v1 = 4;
    uint8_t status = 0;
    if (winner->roundStats.game_score == game_limit_1v1 || loser->roundStats.game_score == game_limit_1v1) {
        status = 255;
        lock();
    }

    NP_GameScore score;
    score.scores.push_back({winner->id, 0, static_cast<uint8_t>(winner->roundStats.game_score), status});
    score.scores.push_back({winner->id, 0, static_cast<uint8_t>(loser->roundStats.game_score), status});

    PM packet;
    packet.write(score);

    sendPacket(packet);
    timeBetweenRounds = sf::seconds(3);
}

void Room::playerDied(Client& died) {
    died.roundStats.position = playersAlive;
    playersAlive--;
    lineSendAdjust.addAdjust(died, clients, playersAlive);
    if (playersAlive < 2) endround = true;
}

void Room::setInactive() {
    if (!active) return;
    active = false;
    if (round) {
        endround = true;
        checkIfRoundEnded();
    } else
        endRound();
    round = false;
    countdown = false;
    start.restart();
}

void Room::setActive() {
    if (locked) return;
    active = true;
    countdown = false;
}

void Room::lock() {
    active = false;
    locked = true;
}

void Room::sendGameData(sf::UdpSocket& udp) {
    for (auto&& client : clients) {
        if (client->datavalid) client->sendGameData(udp);

        sendLines(*client, client->sendLinesOut());
    }
}

void Room::makeCountdown() {
    if (!round) {
        if (countdown) {
            sf::Time t = start.getElapsedTime();
            if (t > sf::seconds(3)) {
                start.restart();
                sendPacketToActive(PM::make<NP_CountdownStop>(static_cast<uint8_t>(1)));
                auto packet = PM::make<NP_CountdownStop>(static_cast<uint8_t>(0));
                sendPacketToAway(packet);
                sendPacketToSpectators(packet);
                startGame();
            } else if (t > countdownTime) {
                countdownTime += sf::milliseconds(200);
                for (auto& client : clients) {
                    if (!client->away) {
                        client->countDown(t);
                    }
                }
            }
        } else if (timeBetweenRounds != sf::seconds(0) && start.getElapsedTime() > timeBetweenRounds)
            startCountdown();
        else {
            bool allready = true;
            for (auto&& client : clients)
                if (!client->away && !client->ready) allready = false;
            if (allready && !waitForReplay) startCountdown();
        }
    }
}

void Room::checkIfRoundEnded() {
    if (round) {
        if (endround) {
            for (auto&& winner : clients)
                if (winner->alive) {
                    winner->alive = false;
                    winner->roundStats.position = 1;
                    winner->sendPacket(PM::make<NP_YouWon>());

                    auto packet = PM::make<NP_PlayerPosition>(winner->id, winner->roundStats.position);
                    sendPacket(packet);
                    sendPacketToSpectators(packet);

                    scoreRound();
                    transfearScore();

                    if (!winner->isHuman()) winner->makeWinner();
                    break;
                }
            endRound();
        }
    }
}

void Room::sendLines(Client& client, uint16_t amount) {
    uint16_t amountbefore = amount;
    amount = client.hcp.send(amount);
    if (amountbefore) cout << client.id << " sending " << amountbefore << " -> " << amount << endl;
    if (playersAlive == 1 || !amount) return;

    float lineAdjust = lineSendAdjust.getAdjust(client.roundStats.score.lines_sent, amount);

    client.roundStats.score.lines_sent += amount;
    client.roundStats.linesAdjusted += lineAdjust;
    float actualSend = amount - lineAdjust;
    actualSend /= playersAlive - 1;

    distributeLines(client.id, actualSend);
}

void Room::distributeLines(uint16_t senderid, float amount) {
    for (auto& client : clients)
        if (client->id != senderid && client->alive) {
            client->roundStats.incLines += amount;
            client->sendLines();
        }
}

void Room::sendPacket(PM& packet) {
    sendPacketToPlayers(packet);
    sendPacketToSpectators(packet);
}

void Room::sendPacketToPlayers(PM& packet) {
    for (auto& client : clients) client->sendPacket(packet);
}

void Room::sendPacketToSpectators(PM& packet) {
    for (auto& spectator : spectators) spectator->sendPacket(packet);
}

void Room::sendPacketToAway(PM& packet) {
    for (auto& client : clients) {
        if (client->away) client->sendPacket(packet);
    }
}

void Room::sendPacketToActive(PM& packet) {
    for (auto& client : clients) {
        if (!client->away) client->sendPacket(packet);
    }
}

bool Room::onlyBots() {
    for (auto& client : clients)
        if (client->isHuman()) return false;

    return spectators.empty();
}

//////////////////////////////////////////////
//					FFA						//
//////////////////////////////////////////////

void FFARoom::scoreRound() {
    if (currentPlayers + leavers.size() < 2) return;

    std::vector<Client*> inround;
    float avgrank = 0;

    for (auto& client : clients) {
        if (client->roundStats.position) {
            client->roundStats.game_score += currentPlayers - client->roundStats.position;
            if (client->stats.ffaRank && !client->guest) {
                inround.push_back(client);
                avgrank += client->stats.ffaRank;
            }
        }
    }
    for (auto& client : leavers) {
        if (client.roundStats.position && client.stats.ffaRank) {
            inround.push_back(&client);
            avgrank += client.stats.ffaRank;
        }
    }
    float playersinround = inround.size();
    if (!playersinround) return;
    avgrank /= playersinround;

    std::sort(inround.begin(), inround.end(), [](Client* c1, Client* c2) { return c1->roundStats.position < c2->roundStats.position; });

    int i = 0;
    for (auto client : inround) {
        float pointcoff = ((((float)i + 1) / (float)playersinround) - 1.0 / (float)playersinround - (1.0 - 1.0 / (float)playersinround) / 2.0) *
                          (-1.0 / ((1.0 - 1.0 / (float)playersinround) / 2.0));
        pointcoff += (client->stats.ffaRank - avgrank) * 0.05 + 0.2;
        client->stats.ffaPoints += 100 * pointcoff * (client->stats.ffaRank / 5.0);
        client->stats.updateFFARank();

        i++;
    }
}

//////////////////////////////////////////////
//					Hero					//
//////////////////////////////////////////////

void HeroRoom::scoreRound() {
    if (currentPlayers + leavers.size() < 2) return;

    for (auto winner : clients)
        if (winner->roundStats.position) {
            for (auto loser : clients)
                if (loser->roundStats.position && winner->roundStats.position < loser->roundStats.position) eloResults.addResult(*winner, *loser, 2);
            for (auto& loser : leavers)
                if (loser.roundStats.position && winner->roundStats.position < loser.roundStats.position) eloResults.addResult(*winner, loser, 2);
        }
    for (auto& winner : leavers)
        if (winner.roundStats.position) {
            for (auto loser : clients)
                if (loser->roundStats.position && winner.roundStats.position < loser->roundStats.position) eloResults.addResult(winner, *loser, 2);
            for (auto& loser : leavers)
                if (loser.roundStats.position && winner.roundStats.position < loser.roundStats.position) eloResults.addResult(winner, loser, 2);
        }

    eloResults.calculateResults();
}

//////////////////////////////////////////////
//					1vs1					//
//////////////////////////////////////////////

void VSRoom::scoreRound() { score1vs1Round(); }

//////////////////////////////////////////////
//					Casual					//
//////////////////////////////////////////////

//////////////////////////////////////////////
//					Tournament				//
//////////////////////////////////////////////

void TournamentRoom::scoreRound() {
    if (tournamentGame == nullptr) return;
    for (auto&& client : clients)
        if (client->roundStats.position == 1) {
            if (tournamentGame->player1->id == client->id) {
                if (tournamentGame->p1won()) lock();
            } else if (tournamentGame->player2->id == client->id) {
                if (tournamentGame->p2won()) lock();
            }
            score1vs1Round();
            return;
        }
}

//////////////////////////////////////////////
//					Challenge				//
//////////////////////////////////////////////
