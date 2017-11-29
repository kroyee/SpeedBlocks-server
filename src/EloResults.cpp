#include "EloResults.h"
#include "Client.h"

float eloExpected(float pointsA, float pointsB) { //Gives A's ExpectedPoints
	return 1.0 / (1.0 + pow(10.0, (pointsB - pointsA) / 400.0)); // FLOAT-EXCEPTION?
}

void EloResults::addResult(Client& winner, Client& loser, sf::Uint8 type) {
	Result newresult;
	newresult.winner = &winner;
	newresult.loser = &loser;
	newresult.type = type;

	if (type == 1) {
		newresult.winner_expected = eloExpected(winner.stats.vsPoints, loser.stats.vsPoints);
		newresult.loser_expected = eloExpected(loser.stats.vsPoints, winner.stats.vsPoints);
	}
	else if (type == 2) {
		newresult.winner_expected = eloExpected(winner.stats.heroPoints, loser.stats.heroPoints);
		newresult.loser_expected = eloExpected(loser.stats.heroPoints, winner.stats.heroPoints);
	}
	else
		return;

	result.push_back(newresult);
}

void EloResults::calculateResults() {
	while (result.size()) {
		if (result.front().type == 1) {
			result.front().winner->stats.vsPoints += 20 * (1 - result.front().winner_expected);
			result.front().loser->stats.vsPoints += 20 * (0 - result.front().loser_expected);
		}
		else if (result.front().type == 2) {
			result.front().winner->stats.heroPoints += 20 * (1 - result.front().winner_expected);
			result.front().loser->stats.heroPoints += 20 * (0 - result.front().loser_expected);
		}

		result.pop_front();
	}
}