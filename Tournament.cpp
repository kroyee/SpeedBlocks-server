#include "Tournament.h"
#include <iostream>
#include "Connections.h"
using std::cout;
using std::endl;

Results::Results() : p1_sets(0), p2_sets(0) {}

Participant::Participant() : id(0), name("") {}

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
		sf::Uint8 tmp = 0;
		result.p1_rounds.push_back(tmp);
		result.p2_rounds.push_back(tmp);
	}
	result.p1_rounds[activeSet]++;
	if (result.p1_rounds[activeSet] == rounds) {
		result.p1_sets++;
		activeSet++;
		if (activeSet == sets) {
			status = 4;
			sendResults();
			decideGame();
			return true;
		}
	}
	status = 3;
	sendResults();
	return false;
}

bool Node::p2won() {
	if (result.p2_rounds.size() == activeSet) {
		sf::Uint8 tmp = 0;
		result.p1_rounds.push_back(tmp);
		result.p2_rounds.push_back(tmp);
	}
	result.p2_rounds[activeSet]++;
	if (result.p2_rounds[activeSet] == rounds) {
		result.p2_sets++;
		activeSet++;
		if (activeSet == sets) {
			status = 4;
			sendResults();
			decideGame();
			return true;
		}
	}
	status = 3;
	sendResults();
	return false;
}

void Node::decideGame() {
	if (nextgame == nullptr) {
		tournament.status = 3;
		tournament.sendStatus();
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
	}
}

void Node::sendResults(bool asPart) {
	if (!asPart) {
		tournament.conn.packet.clear();
		sf::Uint8 packetid = 27, part = 4;
		tournament.conn.packet << packetid << tournament.id << part << id << status;
		if (player1 != nullptr)
			tournament.conn.packet << player1->id << player1->name;
		else
			tournament.conn.packet << (sf::Uint16)0;
		if (player2 != nullptr)
			tournament.conn.packet << player2->id << player2->name;
		else
			tournament.conn.packet << (sf::Uint16)0;
	}
	tournament.conn.packet << result.p1_sets << result.p2_sets;

	tournament.conn.packet << (sf::Uint8)result.p1_rounds.size();
	for (unsigned int i=0; i<result.p1_rounds.size(); i++)
		tournament.conn.packet << result.p1_rounds[i];

	tournament.conn.packet << (sf::Uint8)result.p2_rounds.size();
	for (unsigned int i=0; i<result.p2_rounds.size(); i++)
		tournament.conn.packet << result.p2_rounds[i];

	if (!asPart)
		tournament.sendToTournamentObservers();
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

Tournament::Tournament(Connections& _conn) : conn(_conn), players(0), status(0) {}

bool Tournament::addPlayer(Client& client) {
	for (auto&& player : participants)
		if (player.id == client.id)
			return false;
	Participant par;
	par.id = client.id;
	par.name = client.name;
	participants.push_back(par);
	players++;
	return true;
}

bool Tournament::addPlayer(const sf::String& name, sf::Uint16 id) {
	for (auto&& player : participants)
		if (player.id == id)
			return false;
	Participant par;
	par.id = id;
	par.name = name;
	participants.push_back(par);
	players++;
	return true;
}

bool Tournament::removePlayer(sf::Uint16 id) {
	for (auto it = participants.begin(); it != participants.end(); it++)
		if (it->id == id) {
			it = participants.erase(it);
			players--;
			return true;
		}
	return false;
}

void Tournament::addObserver(Client& client) {
	for (auto&& observer : keepUpdated)
		if (observer->id == client.id)
			return;
	keepUpdated.push_back(&client);
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
	/*bool slotfound;
	for (auto&& player : participants) {
		slotfound=false;
		for (auto&& game : bracket.games)
			if (game.depth == bracket.depth) {
				if (game.player1 == nullptr) {
					game.player1 = &player;
					slotfound=true;
					break;
				}
			}
		if (slotfound)
			continue;
		for (auto&& game : bracket.games)
			if (game.depth == bracket.depth) {
				if (game.player2 == nullptr) {
					game.player2 = &player;
					break;
				}
			}
	}*/


	int amountinrow = 1*pow(2, bracket.depth-1);
	int start = bracket.games.size()-amountinrow;
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
	conn.packet.clear();
	sf::Uint8 packetid = 23;
	conn.packet << packetid;

	conn.packet << id << rounds << sets;

	sendParticipantList(true);
	sendModeratorList(true);
	sendStatus(true);

	if (status > 0)
		sendGames(true);
}

void Tournament::sendParticipantList(bool asPart) {
	if (!asPart) {
		conn.packet.clear();
		sf::Uint8 packetid = 27, part = 0;
		conn.packet << packetid << id << part;
	}
	conn.packet << players;
	for (auto&& player : participants)
		conn.packet << player.id << player.name;

	if (!asPart)
		sendToTournamentObservers();
}

void Tournament::sendModeratorList(bool asPart) {
	if (!asPart) {
		conn.packet.clear();
		sf::Uint8 packetid = 27, part = 1;
		conn.packet << packetid << id << part;
	}
	conn.packet << (sf::Uint8)moderator_list.size();
	for (auto&& mod_id : moderator_list)
		conn.packet << mod_id;

	if (!asPart)
		sendToTournamentObservers();
}

void Tournament::sendStatus(bool asPart) {
	if (!asPart) {
		conn.packet.clear();
		sf::Uint8 packetid = 27, part = 2;
		conn.packet << packetid << id << part;
	}
	conn.packet << status;

	if (!asPart)
		sendToTournamentObservers();
}

void Tournament::sendGames(bool asPart) {
	if (!asPart) {
		conn.packet.clear();
		sf::Uint8 packetid = 27, part = 3;
		conn.packet << packetid << id << part << status;
	}
	for (auto&& game : bracket.games) {
		conn.packet << game.id << game.depth << game.status;
		if (game.player1 != nullptr)
			conn.packet << (sf::Uint8)1 << game.player1->id << game.player1->name;
		else if (game.p1game != nullptr)
			conn.packet << (sf::Uint8)2 << game.p1game->id;
		else
			conn.packet << (sf::Uint8)3;
		if (game.player2 != nullptr)
			conn.packet << (sf::Uint8)1 << game.player2->id << game.player2->name;
		else if (game.p2game != nullptr)
			conn.packet << (sf::Uint8)2 << game.p2game->id;
		else
			conn.packet << (sf::Uint8)3;
		if (game.status > 2)
			game.sendResults(true);
	}

	if (!asPart)
		sendToTournamentObservers();
}

void Tournament::sendToTournamentObservers() {
	for (auto&& client : keepUpdated) {
		conn.status = client->socket.send(conn.packet);
		if (conn.status != sf::Socket::Done)
			cout << "Error sending TCP packet to tournament observer " << id << endl;
	}
}