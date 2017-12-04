#include "gameField.h"
#include "GameSignals.h"
#include <SFML/Graphics.hpp>
#include <iostream>
using std::cout;
using std::endl;
using std::to_string;

BasicField::BasicField() {}

bool BasicField::possible() {
    for (int x=0; x<4; x++)
        for (int y=0; y<4; y++)
            if (piece.grid[y][x]) {
                if (piece.posX+x<0 || piece.posX+x>9 || piece.posY+y<0 || piece.posY+y>21)
                    return false;
                if (square[piece.posY+y][piece.posX+x])
                    return false;
            }
    return true;
}

bool BasicField::mRight() {
    piece.mright();
    if (possible())
        return true;

    piece.mleft();
    return false;
}

bool BasicField::mLeft() {
    piece.mleft();
    if (possible())
        return true;
    
    piece.mright();
    return false;
}

bool BasicField::mDown() {
    piece.mdown();
    if (possible())
        return true;

    piece.mup();
    return false;
}

void BasicField::hd() {
    do { piece.mdown(); }
    while (possible());
    piece.mup();
}

bool BasicField::rcw() {
    piece.rcw();
    if (possible())
        return true;
    if (kickTest())
        return true;

    piece.posX-=2;
    piece.rccw();
    return false;
}

bool BasicField::rccw() {
    piece.rccw();
    if (possible())
        return true;
    if (kickTest())
        return true;

    piece.posX-=2;
    piece.rcw();
    return false;
}

bool BasicField::r180() {
    piece.rccw();
    piece.rccw();
    if (possible())
        return true;
    if (kickTest())
        return true;

    piece.posX-=2;
    piece.rcw();
    piece.rcw();
    return false;
}

bool BasicField::kickTest() {
    piece.posY++; if (possible()) return true;
    piece.posY--; piece.posX--; if (possible()) return true;
    piece.posX+=2; if (possible()) return true;
    piece.posY++; piece.posX-=2; if (possible()) return true;
    piece.posX+=2; if (possible()) return true;
    piece.posX-=3; piece.posY--; if (possible()) return true;
    piece.posX+=4; if (possible()) return true;

    return false;
}

void BasicField::addPiece() {
    for (int x=0; x<4; x++)
        for (int y=0; y<4; y++)
            if (piece.grid[y][x])
                square[piece.posY+y][piece.posX+x]=piece.tile;
}

void BasicField::removeline(short y) {
    for (;y>-1; y--)
        for (int x=0; x<10; x++)
            square[y][x] = square[y-1][x];
    for (int x=0; x<10; x++)
        square[0][x] = 0;
}

sf::Vector2i BasicField::clearlines () {
    sf::Vector2i linescleared(0,0);
    bool rm, gb;
    for (int y=21; y>-1; y--) {
        if (piece.posY+y > 21 || piece.posY+y < 0)
            continue;
        rm=1;
        gb=0;
        for (int x=0; x<10; x++) {
            if (square[piece.posY+y][x] == 8)
                gb=1;
            if (square[piece.posY+y][x] == 0) {
                rm=0;
                break;
            }
        }
        if (rm) {
            removeline(piece.posY+y);
            y++;
            linescleared.x++;
            if (gb)
                linescleared.y++;
        }
    }
    return linescleared;
}

void BasicField::clear() {
    for (int x=0; x<10; x++)
        for (int y=0; y<22; y++)
            square[y][x] = 0;

    piece.piece=7;
}