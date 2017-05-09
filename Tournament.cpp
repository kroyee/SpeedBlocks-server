#include "Tournament.h"
#include <iostream>
#include "Client.h"
using std::cout;
using std::endl;

Results::Results() {}

Participant::Participant() : id(0), name("") {}

Node::Node() {
	player1=nullptr;
	player2=nullptr;
	p1game=nullptr;
	p2game=nullptr;
	nextgame=nullptr;
}

Bracket::Bracket() : players(0), depth(0), gameCount(0), idCount(1) {}

void Bracket::clear() {
	while (games.size())
		games.pop_back();
	players=0;
	depth=0;
	gameCount=0;
	idCount=1;
}

void Bracket::addGame(short _depth, sf::Uint8 sets) {
	Node newgame;
	newgame.depth = _depth;
	newgame.id = idCount;
	newgame.result.set.resize(sets);
	newgame.result.round.resize(sets);
	idCount++;
	games.push_back(newgame);
	depth = _depth;
	gameCount++;
}

Tournament::Tournament() : signupOpen(false), active(false) {}

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
			return true;
		}
	return false;
}

void Tournament::makeBracket() {
	bracket.clear();

	short gamesNeeded = (players-0.5)/2.0;
	gamesNeeded++;

	short currentDepth=1;
	bracket.addGame(currentDepth, sets);

	while (bracket.gameCount < gamesNeeded) {
		bracket.gameCount=0;
		for (auto&& game : bracket.games)
			if (game.depth == currentDepth) {
				bracket.addGame(currentDepth+1, sets);
				linkGames(game, bracket.games.back());
				bracket.addGame(currentDepth+1, sets);
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
	bool slotfound;
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
	}
}

void Tournament::collapseBracket() {
	for (auto it = bracket.games.begin(); it != bracket.games.end(); it++)
		if (it->depth == bracket.depth) {
			if (it->player1 != nullptr && it->player2 == nullptr) {
				if (it->nextgame->p1game->id == it->id) {
					it->nextgame->player1 = it->player1;
					it->nextgame->p1game = nullptr;
				}
				else {
					it->nextgame->player2 = it->player1;
					it->nextgame->p2game = nullptr;
				}
				it = bracket.games.erase(it);
			}
		}
}

void Tournament::printBracket() {
	short currentDepth = 1;
	while (currentDepth <= bracket.depth) {
		for (auto&& game : bracket.games)
			if (game.depth == currentDepth) {
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
		currentDepth++;
	}
}