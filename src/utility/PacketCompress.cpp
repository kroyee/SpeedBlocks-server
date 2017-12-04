#include "PacketCompress.h"
#include "Client.h"

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