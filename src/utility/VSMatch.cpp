#include "VSMatch.h"
#include "Client.h"

void VSMatch::addToQueue(Client& client, const sf::Time& _time) {
	if (client.guest) {
		client.sendSignal(2);
		return;
	}
	client.sendSignal(19);
	for (auto&& inqueue : queue)
		if (inqueue.client->id == client.id)
			return;
	for (auto&& inroom : playing)
		if (inroom.client->id == client.id)
			return;
	if (client.room)
		return;
	client.matchmaking = true;
	QueueClient newclient;
	newclient.client = &client;
	newclient.time = _time;
	queue.push_back(newclient);
}

void VSMatch::removeFromQueue(Client& client) {
	client.sendSignal(20);
	for (auto it = queue.begin(); it != queue.end(); it++)
		if (it->client->id == client.id) {
			queue.erase(it);
			client.matchmaking = false;
			return;
		}
	for (auto it = playing.begin(); it != playing.end(); it++)
		if (it->client->id == client.id) {
			playing.erase(it);
			client.matchmaking = false;
			return;
		}
}

void VSMatch::setPlaying() {
	for (auto it = queue.begin(); it != queue.end(); it++)
		if (it->client->id == player1->id) {
			it->lastOpponent = player2->id;
			playing.splice(playing.end(), queue, it);
			break;
		}
	for (auto it = queue.begin(); it != queue.end(); it++)
		if (it->client->id == player2->id) {
			it->lastOpponent = player1->id;
			playing.splice(playing.end(), queue, it);
			break;
		}
}

void VSMatch::setQueueing(Client& client, const sf::Time& _time) {
	for (auto it = playing.begin(); it != playing.end(); it++)
		if (it->client->id == client.id) {
			it->time = _time;
			queue.splice(queue.end(), playing, it);
			return;
		}
}

bool VSMatch::checkQueue(const sf::Time& _time) {
	if (queue.size() < 2)
		return false;
	sf::Int32 bestmatch = 200;
	for (auto p1 = queue.begin(); p1 != queue.end(); p1++)
		for (auto p2 = p1; p2 != queue.end(); p2++) {
			if (p1 == p2)
				continue;
			
			sf::Int32 match = p1->client->stats.vsPoints - p2->client->stats.vsPoints;
			
			if (match < 0)
				match*=-1;
			
			match -= (_time.asSeconds()*2 - p1->time.asSeconds() - p2->time.asSeconds()) * 40;
			
			if (p1->lastOpponent == p2->client->id)
				match += 200;
			if (p2->lastOpponent == p1->client->id)
				match += 200;

			if (match < bestmatch) {
				player1 = p1->client;
				player2 = p2->client;
				bestmatch = match;
			}
		}

	if (bestmatch < 200)
		return true;

	return false;
}