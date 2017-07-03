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
	// The received time data is milliseconds as an uint16, and here we assume that ~13s has not passed between
	// states and increment by 65535 to store the values as a continous uint32 value
	while (thisFrame->time < lastFrame->time)
		thisFrame->time += 65536;

	thisFrame->received = client.room->start.getElapsedTime().asMilliseconds();
	sf::Int16 timeDiff = thisFrame->received - thisFrame->time;

	if (timeDiff > maxTimeDiff)
		maxTimeDiff = timeDiff;

	if (thisFrame->received - lastFrame->received > 50) {
		if (timeDiff > lastTimeDiff)
			timeDiffDirectionCount++;
		else if (timeDiff < lastTimeDiff)
			timeDiffDirectionCount--;
		lastTimeDiff = timeDiff;
	}

	if (timeDiffDirectionCount > 14) {
		cout << client.name.toAnsiString() << " was kicked from " << client.room->name.toAnsiString() << " for timeDiffDirectionCount violation" << endl;
		client.sendSignal(17, 1);
		client.room->leave(client);
	}

	if (thisFrame->time > lastEveningOutDirectionCount) {
		lastEveningOutDirectionCount+=800;
		if (timeDiffDirectionCount > 0)
			timeDiffDirectionCount--;
		else if (timeDiffDirectionCount < 0)
			timeDiffDirectionCount++;
	}
}

void PlayfieldHistory::clear() {
	timeDiffDirectionCount = 0;
	lastEveningOutDirectionCount = 0;
	lastTimeDiff = 0;
	maxTimeDiff = 0;
	states.clear();
}