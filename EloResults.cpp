#include "EloResults.h"
#include "Client.h"

float eloExpected(float pointsA, float pointsB) { //Gives A's ExpectedPoints
	return 1.0 / (1.0 + pow(10.0, (pointsB - pointsA) / 400.0));
}

void EloResults::addResult(Client& winner, Client& loser, sf::Uint8 type) {
	Result newresult;
	newresult.winner = &winner;
	newresult.loser = &loser;
	newresult.type = type;

	if (type == 1) {
		newresult.winner_expected = eloExpected(winner.s_1vs1points, loser.s_1vs1points);
		newresult.loser_expected = eloExpected(loser.s_1vs1points, winner.s_1vs1points);
	}
	else if (type == 2) {
		newresult.winner_expected = eloExpected(winner.s_heropoints, loser.s_heropoints);
		newresult.loser_expected = eloExpected(loser.s_heropoints, winner.s_heropoints);
	}
	else
		return;

	result.push_back(newresult);
}

void EloResults::calculateResults() {
	while (result.size()) {
		if (result.front().type == 1) {
			result.front().winner->s_1vs1points += 20 * (1 - result.front().winner_expected);
			result.front().loser->s_1vs1points += 20 * (0 - result.front().loser_expected);
		}
		else if (result.front().type == 2) {
			result.front().winner->s_heropoints += 20 * (1 - result.front().winner_expected);
			result.front().loser->s_heropoints += 20 * (0 - result.front().loser_expected);
		}

		result.pop_front();
	}
}