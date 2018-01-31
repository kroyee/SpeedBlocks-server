#include "Tournament.h"
#include <iostream>
#include "Connections.h"
#include "JSONWrap.h"
#include <fstream>
using std::cout;
using std::endl;

Results::Results() : p1_sets(0), p2_sets(0) {}

Participant::Participant() : id(0), name(""), played(false), position(0) {}

Node::Node(Tournament& _tournament) : tournament(_tournament), status(0), activeSet(0) {
	player1=nullptr;
	player2=nullptr;
	p1game=nullptr;
	p2game=nullptr;
	nextgame=nullptr;
	room=nullptr;
}

bool Node::p1won() {
	if (result.p1_rounds.size() == activeSet) {
		uint8_t tmp = 0;
		result.p1_rounds.push_back(tmp);
		result.p2_rounds.push_back(tmp);
	}
	result.p1_rounds[activeSet]++;
	if (result.p1_rounds[activeSet] == rounds) {
		result.p1_sets++;
		activeSet++;
		if (activeSet == sets) {
			decideGame();
			player1->played=true;
			player2->played=true;
			return true;
		}
	}
	status = 3;
	sendResults();
	return false;
}

bool Node::p2won() {
	if (result.p2_rounds.size() == activeSet) {
		uint8_t tmp = 0;
		result.p1_rounds.push_back(tmp);
		result.p2_rounds.push_back(tmp);
	}
	result.p2_rounds[activeSet]++;
	if (result.p2_rounds[activeSet] == rounds) {
		result.p2_sets++;
		activeSet++;
		if (activeSet == sets) {
			decideGame();
			player1->played=true;
			player2->played=true;
			return true;
		}
	}
	status = 3;
	sendResults();
	return false;
}

void Node::winByWO(uint8_t player) {
	result.p1_rounds.clear();
	result.p2_rounds.clear();
	uint8_t score=0;
	result.p1_rounds.push_back(score);
	result.p2_rounds.push_back(score);
	if (player == 1) {
		result.p1_sets=1;
		result.p2_sets=0;
		player1->played=true;
	}
	else {
		result.p2_sets=1;
		result.p1_sets=0;
		player2->played=true;
	}
	decideGame();
	sendScore();
}

void Node::setPosition() {
	if (depth == 1) {
		if (result.p1_sets > result.p2_sets) {
			player1->position=1;
			player2->position=2;
		}
		else {
			player2->position=1;
			player1->position=2;
		}
	}
	else {
		if (result.p1_sets > result.p2_sets)
			player2->position=depth+1;
		else
			player1->position=depth+1;
	}
}

void Node::decideGame() {
	status = 4;
	if (result.p1_sets == result.p2_sets) {
		status = 3;
		sendResults();
		return;
	}
	sendResults();
	setPosition();

	if (nextgame == nullptr) {
		tournament.status = 3;
		tournament.sendStatus();
		if (tournament.grade != 5)
			tournament.thread = new std::thread(&Tournament::scoreTournament, &tournament);
		return;
	}
	
	if (result.p1_sets > result.p2_sets) {
		if (nextgame->p1game->id == id)
			nextgame->player1 = player1;
		else
			nextgame->player2 = player1;
	}
	else {
		if (nextgame->p1game->id == id)
			nextgame->player1 = player2;
		else
			nextgame->player2 = player2;
	}
	
	if (nextgame->player1 != nullptr && nextgame->player2 != nullptr) {
		nextgame->status = 2;
		nextgame->sendResults();
		nextgame->sendReadyAlert();
		nextgame->player1->waitingTime = sf::seconds(0);
		nextgame->player2->waitingTime = sf::seconds(0);
		nextgame->player1->sentWaitingTime = 6;
		nextgame->player2->sentWaitingTime = 6;
	}
}

void Node::sendResults(bool asPart) {
	if (!asPart) {
		tournament.packet.clear();
		uint8_t packetid = 27, part = 4;
		tournament.packet << packetid << tournament.id << part << id << status;
		if (player1 != nullptr)
			tournament.packet << player1->id << player1->name;
		else
			tournament.packet << (uint16_t)0;
		if (player2 != nullptr)
			tournament.packet << player2->id << player2->name;
		else
			tournament.packet << (uint16_t)0;
	}
	tournament.packet << result.p1_sets << result.p2_sets;

	tournament.packet << (uint8_t)result.p1_rounds.size();
	for (unsigned int i=0; i<result.p1_rounds.size(); i++)
		tournament.packet << result.p1_rounds[i];

	tournament.packet << (uint8_t)result.p2_rounds.size();
	for (unsigned int i=0; i<result.p2_rounds.size(); i++)
		tournament.packet << result.p2_rounds[i];

	if (!asPart)
		tournament.sendToTournamentObservers();
}

void Node::sendScore() {
	tournament.packet.clear();
	uint8_t packetid = 24;
	tournament.packet << packetid << player1->id << player2->id << result.p1_sets << result.p2_sets;
	if (status == 4)
		tournament.packet << (uint8_t)255;
	else if (result.p1_rounds.size() > activeSet)
		tournament.packet << result.p1_rounds[activeSet];
	else
		tournament.packet << (uint8_t)0;


	if (result.p2_rounds.size() > activeSet)
		tournament.packet << result.p2_rounds[activeSet];
	else
		tournament.packet << (uint8_t)0;

	if (room != nullptr)
		room->sendPacket(tournament.packet);
}

void Node::sendReadyAlert() {
	if (player1 == nullptr || player2 == nullptr)
		return;
	for (auto&& client : tournament.conn.clients)
		if (client.id == player1->id || client.id == player2->id)
			client.sendSignal(1, tournament.id);
}

void Node::sendWaitTime(uint16_t waitTime, uint8_t player) {
	uint16_t sendTime=5;
	while (waitTime > 60) {
		waitTime-=60;
		sendTime--;
	}
	if (player == 1) {
		if (sendTime < player1->sentWaitingTime) {
			room->clients.front()->sendSignal(3, sendTime);
			room->sendSignalToSpectators(3, sendTime);
			player1->sentWaitingTime = sendTime;
		}
	}
	else {
		if (sendTime < player2->sentWaitingTime) {
			room->clients.front()->sendSignal(3, sendTime);
			room->sendSignalToSpectators(3, sendTime);
			player2->sentWaitingTime = sendTime;
		}
	}
}

void Node::resetWaitTimeSent() {
	if (player1 != nullptr)
		player1->sentWaitingTime = 6;
	if (player2 != nullptr)
		player2->sentWaitingTime = 6;
}

Bracket::Bracket() : players(0), depth(0), gameCount(0), idCount(1) {}

void Bracket::clear() {
	games.clear();
	players=0;
	depth=0;
	gameCount=0;
	idCount=1;
}

void Bracket::addGame(short _depth, Tournament& tournament) {
	Node newgame(tournament);
	newgame.depth = _depth;
	newgame.id = idCount;
	newgame.sets = tournament.sets;
	newgame.rounds = tournament.rounds;
	idCount++;
	games.push_back(newgame);
	depth = _depth;
	gameCount++;
}

void Bracket::sendAllReadyAlerts() {
	for (auto&& game : games)
		if (game.status == 2)
			game.sendReadyAlert();
}

Tournament::Tournament(Connections& _conn) :
conn(_conn), players(0), startingTime(0), status(0), scoreSent(false),
scoreSentFailed(false), updated(false), notify(false) {}

bool Tournament::addPlayer(Client& client) {
	for (auto&& player : participants)
		if (player.id == client.id)
			return false;
	Participant par;
	par.id = client.id;
	par.name = client.name;
	participants.push_back(par);
	players++;
	updated=true;
	conn.lobby.tournamentsUpdated=true;
	return true;
}

bool Tournament::addPlayer(const sf::String& name, uint16_t id) {
	for (auto&& player : participants)
		if (player.id == id)
			return false;
	Participant par;
	par.id = id;
	par.name = name;
	participants.push_back(par);
	players++;
	updated=true;
	conn.lobby.tournamentsUpdated=true;
	return true;
}

bool Tournament::removePlayer(uint16_t id) {
	for (auto it = participants.begin(); it != participants.end(); it++)
		if (it->id == id) {
			it = participants.erase(it);
			players--;
			updated=true;
			conn.lobby.tournamentsUpdated=true;
			return true;
		}
	return false;
}

void Tournament::addObserver(Client& client) {
	for (auto&& observer : keepUpdated)
		if (observer->id == client.id)
			return;
	client.tournament = this;
	keepUpdated.push_back(&client);
}

void Tournament::removeObserver(Client& client) {
	for (auto it = keepUpdated.begin(); it != keepUpdated.end(); it++)
		if ((*it)->id == client.id) {
			client.tournament = nullptr;
			keepUpdated.erase(it);
			return;
		}
}

void Tournament::setStartingTime(short days, short hours, short minutes) {
	time_t timetostart = time(NULL);
	timetostart -= timetostart % 86400;
	timetostart += 86400*days;
	timetostart += 3600*hours;
	timetostart += 60*minutes;
	startingTime = timetostart;
}

void Tournament::makeBracket() {
	bracket.clear();

	short gamesNeeded = (players-0.5)/2.0;
	gamesNeeded++;

	short totalsize = 0;
	short amountinrow = 1;
	while (amountinrow <= gamesNeeded) {
		totalsize+=amountinrow;
		amountinrow*=2;
	}
	totalsize+=amountinrow;

	bracket.games.reserve(totalsize);

	short currentDepth=1;
	bracket.addGame(currentDepth, *this);

	while (bracket.gameCount < gamesNeeded) {
		bracket.gameCount=0;
		for (auto&& game : bracket.games)
			if (game.depth == currentDepth) {
				bracket.addGame(currentDepth+1, *this);
				linkGames(game, bracket.games.back());
				bracket.addGame(currentDepth+1, *this);
				linkGames(game, bracket.games.back());
			}
		currentDepth++;
	}
}

void Tournament::linkGames(Node& game1, Node& game2) {
	if (game1.p1game == nullptr)
		game1.p1game = &game2;
	else
		game1.p2game = &game2;

	game2.nextgame = &game1;
}

void Tournament::putPlayersInBracket() {
	int amountinrow = 1*pow(2, bracket.depth-1);
	/*int start = bracket.games.size()-amountinrow;
	int step = amountinrow/2.0;
	unsigned int index = start;
	bool slot1=true;
	for (auto&& player : participants) {
		if (slot1)
			while (bracket.games[index].player1 != nullptr) {
				index+=step;
				if (index >= bracket.games.size()) {
					if (step < 2) {
						index=start;
						step=amountinrow/2.0;
						slot1=false;
						break;
					}
					step/=2;
					index=start;
				}
			}
		if (slot1)
			bracket.games[index].player1 = &player;
		if (!slot1)
			while (bracket.games[index].player2 != nullptr) {
				index+=step;
				if (index >= bracket.games.size()) {
					if (step < 2)
						break;
					step/=2;
					index=start;
				}
			}
		if (!slot1)
			bracket.games[index].player2 = &player;
	}*/
	int counter=0;
	std::random_shuffle(participants.begin(), participants.end());
	for (auto&& player : participants) {
		int z=1, x=counter;
		float sum=0;

		while(x!=0) {
			if(x&1)
			sum+=1.0/pow(2,z);

			z++;
			x=x>>1;
		}

		int rattmatch=sum*amountinrow + amountinrow - 1;
		int rattpos = sum*amountinrow*2;
		rattpos = rattpos%2;
		if (rattpos)
			bracket.games[rattmatch].player2 = &player;
		else
			bracket.games[rattmatch].player1 = &player;

		counter++;
	}

	setGameStatus();
	collapseBracket();
}

void Tournament::setGameStatus() {
	for (auto&& game : bracket.games) {
		if (game.p1game == nullptr || game.p2game == nullptr)
			game.status = 0;
		else
			game.status = 1;
		if (game.player1 != nullptr && game.player2 != nullptr)
			game.status = 2;
	}
}

void Tournament::collapseBracket() {
	for (auto&& game : bracket.games)
		if (game.depth == bracket.depth) {
			if (game.player1 != nullptr && game.player2 == nullptr) {
				if (game.nextgame->p1game->id == game.id)
					game.nextgame->player1 = game.player1;
				else {
					game.nextgame->player2 = game.player1;
					if (game.nextgame->player1 != nullptr && game.nextgame->player2 != nullptr)
						game.nextgame->status = 2;
				}
			}
		}
}

void Tournament::printBracket() {
	short currentDepth = 1;
	for (auto&& game : bracket.games) {
		if (game.depth != currentDepth) {
			cout << endl;
			currentDepth++;
		}
		cout << game.id << ": ";
		if (game.player1 != nullptr)
			cout << game.player1->name.toAnsiString();
		else if (game.p1game != nullptr)
			cout << game.p1game->id;
		else
			cout << "Empty";
		cout << " vs ";
		if (game.player2 != nullptr)
			cout << game.player2->name.toAnsiString();
		else if (game.p2game != nullptr)
			cout << game.p2game->id;
		else
			cout << "Empty";
		cout << " - ";
	}
	cout << endl;
}

void Tournament::sendTournament() {
	packet.clear();
	uint8_t packetid = 23;
	packet << packetid;

	packet << id << grade << rounds << sets << (double)startingTime;

	sendParticipantList(true);
	sendModeratorList(true);
	sendStatus(true);

	if (status > 0)
		sendGames(true);

	conn.sender->sendPacket(packet);
}

void Tournament::sendParticipantList(bool asPart) {
	if (!asPart) {
		packet.clear();
		uint8_t packetid = 27, part = 0;
		packet << packetid << id << part;
	}
	packet << players;
	for (auto&& player : participants)
		packet << player.id << player.name;

	if (!asPart)
		sendToTournamentObservers();
}

void Tournament::sendModeratorList(bool asPart) {
	if (!asPart) {
		packet.clear();
		uint8_t packetid = 27, part = 1;
		packet << packetid << id << part;
	}
	packet << (uint8_t)moderator_list.size();
	for (auto&& mod_id : moderator_list)
		packet << mod_id;

	if (!asPart)
		sendToTournamentObservers();
}

void Tournament::sendStatus(bool asPart) {
	if (!asPart) {
		packet.clear();
		uint8_t packetid = 27, part = 2;
		packet << packetid << id << part;
	}
	packet << status;

	if (!asPart)
		sendToTournamentObservers();
}

void Tournament::sendGames(bool asPart) {
	if (!asPart) {
		packet.clear();
		uint8_t packetid = 27, part = 3;
		packet << packetid << id << part << status;
	}
	for (auto&& game : bracket.games) {
		packet << game.id << game.depth << game.status;
		if (game.player1 != nullptr)
			packet << (uint8_t)1 << game.player1->id << game.player1->name;
		else if (game.p1game != nullptr)
			packet << (uint8_t)2 << game.p1game->id;
		else
			packet << (uint8_t)3;
		if (game.player2 != nullptr)
			packet << (uint8_t)1 << game.player2->id << game.player2->name;
		else if (game.p2game != nullptr)
			packet << (uint8_t)2 << game.p2game->id;
		else
			packet << (uint8_t)3;
		if (game.status > 2)
			game.sendResults(true);
	}

	if (!asPart)
		sendToTournamentObservers();
}

void Tournament::sendToTournamentObservers() {
	for (auto&& client : keepUpdated) {
		conn.status = client->socket->send(packet);
		if (conn.status != sf::Socket::Done)
			cout << "Error sending TCP packet to tournament observer " << id << endl;
	}
}

void Tournament::startTournament() {
	for (auto&& player : participants) {
		player.waitingTime = sf::seconds(0);
		player.sentWaitingTime = 6;
	}
	if (status == 0) {
		if (players < 2) {
			status = 4;
			notify = true;
			sendStatus();
			return;
		}
		makeBracket();
		putPlayersInBracket();
		status = 2;
		sendGames();
		bracket.sendAllReadyAlerts();
	}
	else if (status == 1) {
		status = 2;
		sendStatus();
		bracket.sendAllReadyAlerts();
	}
}

void Tournament::checkIfStart() {
	if (status < 2)
		if (startingTime) {
			if (time(NULL) > startingTime)
				startTournament();
			if (!notify)
				if (time(NULL) > startingTime-1200) {
					notify=true;
					JSONWrap jwrap;
					sf::String cont = "<@&384787999675645963> Tournament Notification%" + name + " is starting in 20 minutes%Participants: ";
					for (auto& player : participants)
						cont += player.name + " ";
					jwrap.addPair("content", cont);
					jwrap.addPair("username", "SB_Notify");
					jwrap.addPair("key", conn.serverkey);
					cout << jwrap.sendPost("/forward_to_discord.php").getBody() << endl;
				}
		}
}

void Tournament::checkWaitTime() {
	if (status == 2)
		for (auto&& game : bracket.games)
			if (game.status == 2 || game.status == 3)
				if (game.room != nullptr)
					if (game.room->currentPlayers == 1) {
						if (game.room->clients.front()->id == game.player1->id) {
							game.player1->waitingTime += conn.serverClock.getElapsedTime() - waitingTime;
							uint16_t waitTime = game.player1->waitingTime.asSeconds();
							if (waitTime > 300)
								game.winByWO(1);
							else
								game.sendWaitTime(waitTime, 1);
						}
						else {
							game.player2->waitingTime += conn.serverClock.getElapsedTime() - waitingTime;
							uint16_t waitTime = game.player2->waitingTime.asSeconds();
							if (waitTime > 300)
								game.winByWO(2);
							else
								game.sendWaitTime(waitTime, 2);
						}
					}
	waitingTime = conn.serverClock.getElapsedTime();
}

void Tournament::checkIfScoreWasSent() {
	if (scoreSent)
		if (thread->joinable()) {
			thread->join();
			delete thread;
			scoreSent=false;
		}
	if (scoreSentFailed)
		if (thread->joinable()) {
			thread->join();
			scoreSentFailed=false;
			delete thread;
			thread = new std::thread(&Tournament::scoreTournament, this);
		}
}

void Tournament::scoreTournament() {
	JSONWrap jwrap;
	jwrap.addPair("depth", bracket.depth);
	jwrap.addPair("grade", grade);
	for (auto participant : participants)
		if (participant.played)
			jwrap.addPair(std::to_string(participant.id), participant.position);

	sf::Http::Response response = jwrap.sendPost("/report_tournament.php");
	if (response.getStatus() == sf::Http::Response::Ok) {
		cout << "Tournament " << id << ": " << response.getBody() << endl;
		scoreSent=true;
	}
    else {
        std::cout << "scoreTournament failed request" << std::endl;
        scoreSentFailed = true;
    }
}

void Tournament::save() {
	if (/*!updated || */status != 0)
		return;
	cout << "Saving tournament " << name.toAnsiString() << endl;
	std::ofstream file("Tournaments/" + name);
	if (!file.is_open()) {
		cout << "Failed to save tournament " << name.toAnsiString() << endl;
		return;
	}
	file << name.toAnsiString() << endl << (int)grade << endl;
	file << startingTime << endl << (int)rounds << endl << (int)sets << endl;
	file << moderator_list.size() << endl;
	for (auto mod : moderator_list)
		file << mod << endl;
	file << players << endl;
	for (auto player : participants)
		file << player.id << endl << player.name.toAnsiString() << endl;

	updated=false;
	file.close();
}