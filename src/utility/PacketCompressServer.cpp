#include "PacketCompressServer.h"
#include "AI.h"

PacketCompressServer& PacketCompressServer::operator=(const AI& ai) {
    square = ai.field.square;
    posX = ai.field.piece.posX + 2;
    posY = ai.field.piece.posY;
    piece = ai.field.piece.piece;
    color = ai.field.piece.tile;
    rotation = ai.field.piece.current_rotation;
    nextpiece = ai.nextpiece;
    npcol = ai.npcol;
    nprot = ai.nprot;
    comboText = ai.combo.comboCount;
    pendingText = ai.garbage.count();
    bpmText = ai.bpmCounter.bpm;
    comboTimerCount = ai.combo.timerCount(ai.gameclock.getElapsedTime());
    countdown = ai.countdown;
    return *this;
}