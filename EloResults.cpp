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
		newresult.winner_expected = eloExpected(winner.stats.vspoints, loser.stats.vspoints);
		newresult.loser_expected = eloExpected(loser.stats.vspoints, winner.stats.vspoints);
	}
	else if (type == 2) {
		newresult.winner_expected = eloExpected(winner.stats.heropoints, loser.stats.heropoints);
		newresult.loser_expected = eloExpected(loser.stats.heropoints, winner.stats.heropoints);
	}
	else
		return;

	result.push_back(newresult);
}

void EloResults::calculateResults() {
	while (result.size()) {
		if (result.front().type == 1) {
			result.front().winner->stats.vspoints += 20 * (1 - result.front().winner_expected);
			result.front().loser->stats.vspoints += 20 * (0 - result.front().loser_expected);
		}
		else if (result.front().type == 2) {
			result.front().winner->stats.heropoints += 20 * (1 - result.front().winner_expected);
			result.front().loser->stats.heropoints += 20 * (0 - result.front().loser_expected);
		}

		result.pop_front();
	}
}