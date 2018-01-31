#include "PacketCompress.h"
#include "Client.h"
#include "AI.h"

void PacketCompress::extract(HistoryState& history) {
	tmpcount=0;
	bitcount=0;
	uint8_t counter=0;
	uint8_t endy=0;
	uint8_t temp=0;
	int y;
	getBits(endy, 5);
	for (int c=0; c<endy; c++) {
		for (int x=0; x<10; x++)
			history.square[21-c][x]=8;
		getBits(temp, 4);
		history.square[21-c][temp]=0;
	}
	for (int x=0; x<10; x++) {
		counter=0;
		getBits(counter, 5);
		for (y=0; y<counter; y++)
			history.square[y][x]=0;
		for (; y<22-endy; y++)
			getBits(history.square[y][x], 3);
	}
	uint8_t discard;
	getBits(discard, 4); getBits(discard, 5);
	getBits(history.piece, 3);
	getBits(discard, 5);
	getBits(history.nextpiece, 3);
	getBits(discard, 5);
	getBits(history.combo, 5);
	getBits(history.pending, 8);
	getBits(history.bpm, 8);
	getBits(history.comboTimer, 7);
	getBits(history.countdown, 2);
	if (history.countdown)
		history.time=0;
	else {
		uint8_t smallpart, bigpart;
		getBits(smallpart, 8);
		getBits(bigpart, 8);
		history.time=bigpart*256 + smallpart;
	}
}

void PacketCompress::getBits(uint8_t& byte, uint8_t bits) {
	uint8_t temp=0;
	temp = tmp[tmpcount]>>bitcount | temp;
	bitcount+=bits;
	if (bitcount>7) {
		bitcount-=8;
		tmpcount++;
		if (bitcount>0)
			temp = tmp[tmpcount]<<(bits-bitcount) | temp;
	}
	temp = temp<<(8-bits);
	temp = temp>>(8-bits);
	byte=temp;
}

void PacketCompress::compress(AI& ai) {
	tmp.clear();
	tmpcount=0;
	bitcount=0;
	uint8_t counter = 0;
	int y, endy;
	for (endy=21; endy>=0; endy--) {
		if (ai.field.square[endy][0]==8 || ai.field.square[endy][1]==8)
			counter++;
		else
			break;
	}
	addBits(counter, 5);
	for (y=21; y>endy; y--)
		for (uint8_t x=0; x<10; x++)
			if (ai.field.square[y][x] == 0) {
				addBits(x, 4);
				break;
			}
	for (int x=0; x<10; x++) {
		counter=0;
		for (y=0; y<=endy; y++) {
			if (!ai.field.square[y][x])
				counter++;
			else
				break;
		}
		addBits(counter, 5);
		for (; y<=endy; y++) {
			addBits(ai.field.square[y][x], 3);
		}
	}
	uint8_t posx=0, posy=0;
	posx = ai.field.piece.posX+2; posy = ai.field.piece.posY;
	addBits(posx, 4);
	addBits(posy, 5);
	if (ai.countdown)
		addBits(7, 3);
	else
		addBits(ai.field.piece.piece, 3);
	addBits(ai.field.piece.tile, 3);
	addBits(ai.field.piece.current_rotation, 2);
	addBits(ai.nextpiece, 3);
	addBits(ai.basepiece[ai.nextpiece].tile, 3);
	addBits(ai.nprot, 2);
	addBits(ai.combo.comboCount, 5);
	addBits(ai.garbage.count(), 8);
	uint8_t tmp;
	if (ai.bpmCounter.calcBpm(ai.gameclock.getElapsedTime()) > 255)
		tmp=255;
	else
		tmp = ai.bpmCounter.bpm;
	addBits(tmp, 8);
	addBits(ai.combo.timerCount(ai.gameclock.getElapsedTime()), 7);
	addBits(ai.countdown, 2);
}

void PacketCompress::addBits(uint8_t byte, uint8_t bits) {
	if (tmpcount >= tmp.size())
		tmp.push_back(0);
	tmp[tmpcount] = tmp[tmpcount] | byte<<bitcount;
	bitcount+=bits;
	if (bitcount>7) {
		bitcount-=8;
		tmpcount++;
		if (bitcount>0) {
			tmp.push_back(0);
			tmp[tmpcount] = tmp[tmpcount] | byte>>(bits-bitcount);
		}
	}
}

void PacketCompress::dumpTmp(sf::Packet& packet) {
	for (auto i : tmp)
		packet << i;
}

void PacketCompress::loadTmp(sf::Packet& packet) {
	tmpcount=0;
	tmp.clear();
	uint8_t temp;
	while (packet >> temp)
		tmp.push_back(temp);
}