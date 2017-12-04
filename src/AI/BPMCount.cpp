#include "BPMCount.h"

void BPMCount::addPiece(const sf::Time& _time) {
	std::lock_guard<std::mutex> mute(bpmMutex);
	bpmCount.push_front(_time);
}

uint16_t BPMCount::calcBpm(const sf::Time& _time) {
	std::lock_guard<std::mutex> mute(bpmMutex);
	while (_time > bpmMeasureTiming) {
		bpmMeasureTiming+=sf::milliseconds(100);
		while (bpmCount.size()) {
			if (_time>bpmCount.back()+sf::seconds(5))
				bpmCount.pop_back();
			else
				break;
		}
		oldbpm[oldbpmCount] = bpmCount.size()*12;
		float total=0;
		for(int i=0; i<10; i++)
			total+=oldbpm[i];
		bpm=total/10;
		oldbpmCount++;
		if (oldbpmCount==10)
			oldbpmCount=0;
	}
	return bpm;
}

void BPMCount::clear() {
	std::lock_guard<std::mutex> mute(bpmMutex);
	bpmMeasureTiming=sf::seconds(0);
	oldbpmCount=0;
	bpm=0;
	for (int i=0; i<10; i++)
		oldbpm[i]=0;
	bpmCount.clear();
}