#include "ClientHistory.h"
#include "Room.h"
using std::cout;
using std::endl;

PlayfieldHistory::PlayfieldHistory(Client& _client) : client(_client), timeDiffDirectionCount(0) {}

void PlayfieldHistory::validate() {
	auto thisFrame = states.begin();
	auto lastFrame = states.begin();
	lastFrame++;
	if (lastFrame == states.end() || !client.room)
		return;
	// The received time data is only 0-255 (ms/100 or 0-25,5s) and here we convert it to be stored as a
	// continous ms/100 value from 0~65000 or 0-6500s
	while (thisFrame->time < lastFrame->time)
		thisFrame->time += 256;

	sf::Int8 timeDiff = client.room->start.getElapsedTime().asMilliseconds()/100 - thisFrame->time;

	if (timeDiff > maxTimeDiff)
		maxTimeDiff = timeDiff;

	if (timeDiff > lastTimeDiff)
		timeDiffDirectionCount++;
	else if (timeDiff < lastTimeDiff)
		timeDiffDirectionCount--;

	lastTimeDiff = timeDiff;

	cout << (int)timeDiff << " " << (int)timeDiffDirectionCount << endl;
}

void PlayfieldHistory::clear() {
	timeDiffDirectionCount = 0;
	lastTimeDiff = 0;
	maxTimeDiff = 0;
	states.clear();
}