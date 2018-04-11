#ifndef AI_H
#define AI_H

#include "gameField.h"
#include "randomizer.h"
#include "TestField.h"
#include "BPMCount.h"
#include "Garbage.h"
#include "Combo.h"
#include "DropDelay.h"
#include "GameSignals.h"
#include "StatsHolders.h"
#include "Client.h"
#include "PacketCompress.h"
#include <SFML/Network.hpp>
#include <vector>
#include <deque>
#include <thread>
#include <atomic>
#include <mutex>
#include <array>

class Resources;

enum class Mode { Downstack, Stack };

enum class Competative { None, Low, Medium, High, Super };

class AI : public Client {
public:
	std::array<double, 10> weights, downstackWeights, stackWeights;
	TestField firstMove, secondMove;
	BasicField field;
	basePieces basepiece[7];
	uint8_t piecerotation[7];
	uint8_t colormap[7];

	uint8_t nextpiece, nprot, npcol, offset, countdown=0;
	std::atomic<uint16_t> linesToBeSent;

	sf::Vector2i well2Pos;

	uint16_t gameCount;
	uint16_t score;

	Competative competative = Competative::None;

	float incomingLines;

	Mode mode;
	bool drawMe;
	std::vector<uint8_t> currentMove;
	std::deque<uint8_t> moveQueue;
	std::vector<uint8_t>::iterator moveIterator;

	sf::Time nextmoveTime, movepieceTime, moveTime, finesseTime, updateGameDataTime;

	randomizer rander;
	BPMCount bpmCounter;
	GarbageHandler garbage;
	ComboCounter combo;
	DropDelay pieceDropDelay;

	PacketCompress compressor;

	sf::Clock& gameclock;

	std::atomic<uint8_t> updateField;
	std::mutex gamedataMutex;
	std::atomic<bool> adjustDownMove, movingPiece;

	AI(Room& room);

	void startMove();
	void continueMove();
	bool executeMove();
	void setPiece(int piece);
	void setNextPiece(int piece);

	void startAI();
	void restartGame();
	void addGarbageLine();

	void setMode(Mode, bool vary=false);
	AI& setSpeed(uint16_t speed);
	void updateSpeed() override;
	AI& setCompetative(Competative);

	bool aiThreadRun();

	void startGame();
	void startCountdown();
	void countDown(const sf::Time&) override;
	void endRound() override;

	void delayCheck(const sf::Time& t);
	void setComboTimer(const sf::Time& t);
	void sendLines(sf::Vector2i lines, const sf::Time& t);
	void addGarbage(uint16_t amount, const sf::Time& t);

	void initBasePieces();
	void setPieceColor(short i, uint8_t newcolor);
	std::vector<short> pieceArray();

	void updateGameData();

	void sendLines() override;
	uint16_t sendLinesOut() override;
	void sendGameData(sf::UdpSocket&) override;
	void seed(uint16_t, uint16_t, uint8_t = 0) override;
	void getRoundData(sf::Packet&) override;
};

#endif
