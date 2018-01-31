#include "AI.h"
#include <fstream>
#include <iostream>
using std::cout;
using std::endl;

AI::AI(sf::Clock& _gameclock) :
firstMove(*this),
secondMove(*this),
garbage(data.linesBlocked),
combo(data.maxCombo),
gameclock(_gameclock) {
	movingPiece=false;
	nextmoveTime=sf::seconds(0);
	movepieceTime=sf::seconds(0);
	updateField=0;

	piecerotation[0] = 3;
	piecerotation[1] = 1;
	piecerotation[2] = 3;
	piecerotation[3] = 1;
	piecerotation[4] = 1;
	piecerotation[5] = 2;
	piecerotation[6] = 0;

	initBasePieces();
	setPiece(0);

	downstackWeights[0] = -0.777562;
	downstackWeights[1] = -0.957217;
	downstackWeights[2] = -0.206355;
	downstackWeights[3] = 0.305608;
	downstackWeights[4] = -0.0985396;
	downstackWeights[5] = -0.571009;
	downstackWeights[6] = -0.0826352;
	downstackWeights[7] = -0.268683;
	downstackWeights[8] = 0.01;
	downstackWeights[9] = -0.947217;

	stackWeights[0] = 0.0646257;
	stackWeights[1] = -0.781367;
	stackWeights[2] = -0.079562;
	stackWeights[3] = -0.112896;
	stackWeights[4] = 0.238397;
	stackWeights[5] = -0.136575;
	stackWeights[6] = -0.0488756;
	stackWeights[7] = -0.206737;
	stackWeights[8] = 0.01;
	stackWeights[9] = -0.771367;
}

void AI::startMove() {
	movingPiece=true;
	movepieceTime = nextmoveTime - moveTime;
	currentMove.clear();
	int rotationValue = firstMove.move.rot - basepiece[firstMove.piece.piece].rotation;
	if (rotationValue < 0)
		rotationValue += 4;
	if (rotationValue)
		currentMove.push_back(240+rotationValue);

	if (firstMove.move.use_path) {
		if (firstMove.move.posX > field.piece.posX) for (int i=0; i < firstMove.move.posX-field.piece.posX; i++)
			currentMove.push_back(255);
		else for (int i=0; i < field.piece.posX-firstMove.move.posX; i++)
			currentMove.push_back(254);
		for (auto it = firstMove.move.path.rbegin(); it != firstMove.move.path.rend(); it++) {
			if (*it < 240) {
				for (int i = 0; i < *it; i++)
					currentMove.push_back(252);
			}
			else currentMove.push_back(*it);
		}
	}
	else {
		if (firstMove.move.posX > field.piece.posX) for (int i=0; i < firstMove.move.posX-field.piece.posX; i++)
			currentMove.push_back(255);
		else for (int i=0; i < field.piece.posX-firstMove.move.posX; i++)
			currentMove.push_back(254);
		currentMove.push_back(253);
	}
	moveIterator = currentMove.begin();
	continueMove();
}

void AI::continueMove() {
	if (gameclock.getElapsedTime() <= movepieceTime)
		return;

	while (movingPiece && gameclock.getElapsedTime() > movepieceTime) {
		moveQueue.push_back(*moveIterator);

		if (*moveIterator == 252)
			movepieceTime += sf::milliseconds(finesseTime.asMilliseconds() / 3.0);
		else
			movepieceTime += finesseTime;
		moveIterator++;

		if (moveIterator == currentMove.end()) {
			moveQueue.push_back(230);
			movingPiece=false;

			if (firstMove.totalHeight > 125)
				setMode(Mode::Downstack);
			else if (firstMove.totalHeight < 15)
				setMode(Mode::Stack);
		}

		executeMove();
	}
}

bool AI::executeMove() {
	while (!moveQueue.empty()) {
		if (moveQueue.front() == 255)
			field.mRight();
		else if (moveQueue.front() == 254)
			field.mLeft();
		else if (moveQueue.front() == 253) {
			field.hd();
			adjustDownMove=false;
		}
		else if (moveQueue.front() == 252) {
			if (adjustDownMove)
				adjustDownMove=false;
			else if (field.mDown())
				pieceDropDelay.reset(gameclock.getElapsedTime());
		}
		else if (moveQueue.front() == 241)
			field.rcw();
		else if (moveQueue.front() == 242)
			field.r180();
		else if (moveQueue.front() == 243)
			field.rccw();
		else if (moveQueue.front() == 230) {
			field.hd();
			field.addPiece();

			sendLines(field.clearlines(), gameclock.getElapsedTime());
			data.pieceCount++;

			setPiece(nextpiece);
			setNextPiece(rander.getPiece());

			bpmCounter.addPiece(gameclock.getElapsedTime());

			if (!field.possible()) {
				alive=false;
				return true;
			}
		}
		
		moveQueue.pop_front();
	}

	return false;
}

void AI::setPiece(int piece) {
	field.piece = basepiece[piece];
	field.piece.posX = 3;
	field.piece.posY = 0;
}

void AI::setNextPiece(int piece) {
	nextpiece = piece;
	nprot = basepiece[piece].rotation;
	npcol = basepiece[piece].tile;
}

void AI::startAI() {
	data.clear();
	gameCount=0;
	field.clear();
}

void AI::restartGame() {
	field.clear();
	gameCount++;
	data.clear();
	setMode(Mode::Stack);
	setPiece(0);
	field.piece.piece = 7;
	setNextPiece(rander.getPiece());
	while (nextpiece == 2 || nextpiece == 3)
		setNextPiece(rander.getPiece());
}

void AI::addGarbageLine() {
	for (int y=0; y<21; y++)
		for (int x=0; x<10; x++)
			field.square[y][x]=field.square[y+1][x];
	for (int x=0; x<10; x++)
		field.square[21][x]=8;
	field.square[21][rander.getHole()]=0;
}

void AI::setMode(Mode _mode, bool vary) {
	mode = _mode;
	if (mode == Mode::Downstack)
		weights = downstackWeights;

	else if (mode == Mode::Stack)
		weights = stackWeights;

	if (vary)
		for (int i=0; i<10; i++)
			weights[i] += rander.piece_dist(rander.AI_gen)*0.2 - 0.1;

	firstMove.weights = weights;
	secondMove.weights = weights;
}

void AI::setSpeed(uint16_t _speed) {
	float speed = 60000000.0 / (_speed*1.05);
	moveTime = sf::microseconds(speed);
	finesseTime = sf::microseconds(speed / 15.0);
}

bool AI::playAI() {
	if (executeMove()) {
		alive = false;
		return true;
	}

	if (updateField == 1) {
		firstMove.square = field.square;
		firstMove.setPiece(field.piece.piece);
		//nextpiece = field.nextpiece;
		updateField = 2;
	}

	return false;
}

void AI::aiThreadRun() {
	if (movingPiece)
		continueMove();
	else if (gameclock.getElapsedTime() > nextmoveTime) {
		nextmoveTime += moveTime;
		firstMove.square = field.square;
		firstMove.setPiece(field.piece.piece);

		firstMove.calcHeightsAndHoles();
		if (data.pieceCount < 5)
			setMode(Mode::Stack, true);
		else if (data.pieceCount == 5)
			setMode(Mode::Stack);
		else if (firstMove.totalHeight > 130 || firstMove.highestPoint > 17)
			setMode(Mode::Downstack);
		else if (firstMove.totalHeight < 15)
			setMode(Mode::Stack);

		firstMove.calcHolesBeforePiece();
		float pieceAdjust = (data.pieceCount < 5 ? rander.piece_dist(rander.AI_gen) * 0.5 - 0.25 : 0);
		firstMove.tryAllMoves(secondMove, nextpiece, pieceAdjust);
		startMove();
	}


	if (gameclock.getElapsedTime() > updateGameDataTime)
		updateGameData();
}

void AI::startRound() {
	garbage.clear();
	combo.clear();
	bpmCounter.clear();
	pieceDropDelay.clear();
	incomingLines=0;
	setPiece(nextpiece);
	setNextPiece(rander.getPiece());
	movepieceTime = sf::seconds(0);
	nextmoveTime = sf::seconds(0);
	updateGameDataTime = sf::seconds(0);
	alive=true;
	updateField=0;
	adjustDownMove=false;
}

void AI::startCountdown() {
	restartGame();
	garbage.clear();
	combo.clear();
	bpmCounter.clear();
}

void AI::countDown(int count) {
	updateGameData(count);
}

void AI::endRound(const sf::Time& _time, bool) {
	alive = false;
	data.bpm = static_cast<float>(data.pieceCount) / _time.asSeconds() * 60.0;
}

void AI::delayCheck(const sf::Time& t) {
	if (pieceDropDelay.check(t)) {
		if (field.mDown()) {
			adjustDownMove=true;
			drawMe=true;
			//lockdown=false;
		}
		/*else {
			if (!lockdown)
				lockDownTime=gameclock.getElapsedTime()+sf::milliseconds(400);
			lockdown=true;
		}*/
	}

	uint16_t comboLinesSent = combo.check(t);
	if (comboLinesSent) {
		comboLinesSent = garbage.block(comboLinesSent, t, false);
		data.linesSent += comboLinesSent;
		if (comboLinesSent)
			linesToBeSent+=comboLinesSent;
		drawMe=true;
	}

	data.bpm = bpmCounter.calcBpm(t);


	setComboTimer(t);

	if (garbage.check(t)) {
		addGarbageLine();
		if (!field.piece.posY)
			adjustDownMove=true;
	}

	/*if (lockdown && gameclock.getElapsedTime() > lockDownTime) {
		if (!field.mDown()) {
			addPiece(gameclock.getElapsedTime());
			sendLines(field.clearlines());
			drawMe=true;
			makeNewPiece();
		}
		else
			lockdown=false;
	}*/

	field.offset = garbage.getOffset(t);
}

void AI::setComboTimer(const sf::Time& t) {
	combo.timerCount(t);
}

void AI::sendLines(sf::Vector2i lines, const sf::Time& t) {
	data.garbageCleared+=lines.y;
	data.linesCleared+=lines.x;
	if (lines.x==0) {
		combo.noClear();
		return;
	}
	uint16_t amount = garbage.block(lines.x-1, t);
	data.linesSent += amount;
	if (amount)
		linesToBeSent+=amount;
	combo.increase(t, lines.x);

	setComboTimer(t);
}

void AI::addGarbage(uint16_t amount, const sf::Time& t) {
	garbage.add(amount, t);

	data.linesReceived+=amount;
}

void AI::initBasePieces() {
	std::vector<short> value = pieceArray();

	short vc=0;

	for (int p=0; p<7; p++) {
		basepiece[p].posX=0;
		basepiece[p].posY=0;
		basepiece[p].lpiece=false;
		basepiece[p].rotation=piecerotation[p];
		basepiece[p].current_rotation=0;
		basepiece[p].tile=colormap[p];
		basepiece[p].piece=p;
		for (int y=0; y<4; y++)
			for (int x=0; x<4; x++) {
                basepiece[p].grid[y][x] = value[vc];
				vc++;
			}
        setPieceColor(p, basepiece[p].tile);
	}
	basepiece[4].lpiece=true;
	basepiece[6].lpiece=true;
}

void AI::setPieceColor(short i, uint8_t newcolor) {
    colormap[i] = newcolor;
    basepiece[i].tile = newcolor;
	for (int x=0; x<4; x++)
		for (int y=0; y<4; y++)
			if (basepiece[i].grid[y][x])
				basepiece[i].grid[y][x]=basepiece[i].tile;
}

std::vector<short> AI::pieceArray() {
	std::vector<short> value = {
		0, 4, 0, 0,
		0, 4, 0, 0,
		0, 4, 4, 0,
		0, 0, 0, 0,

		0, 3, 0, 0,
		0, 3, 0, 0,
		3, 3, 0, 0,
		0, 0, 0, 0,

		0, 5, 0, 0,
		0, 5, 5, 0,
		0, 0, 5, 0,
		0, 0, 0, 0,

		0, 7, 0, 0,
		7, 7, 0, 0,
		7, 0, 0, 0,
		0, 0, 0, 0,

		0, 2, 0, 0,
		0, 2, 0, 0,
		0, 2, 0, 0,
		0, 2, 0, 0,

		0, 0, 0, 0,
		1, 1, 1, 0,
		0, 1, 0, 0,
		0, 0, 0, 0,

		0, 0, 0, 0,
		0, 6, 6, 0,
		0, 6, 6, 0,
		0, 0, 0, 0
	};

	return value;
}

void AI::updateGameData(int count) {
	
}