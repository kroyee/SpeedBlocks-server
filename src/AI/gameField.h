#ifndef GAMEFIELD_H
#define GAMEFIELD_H

#include <SFML/Graphics.hpp>
#include "pieces.h"
#include <thread>
#include <atomic>
#include <array>

class Resources;

class BasicField {
public:
    BasicField();

    std::array<std::array<uint8_t, 10>, 22> square;
    basePieces piece;
    uint8_t offset;

    bool possible();

    bool mRight();
    bool mLeft();
    bool mDown();
    void hd();
    bool rcw();
    bool rccw();
    bool r180();
    bool kickTest();

    void addPiece();

    void removeline(short y);
    sf::Vector2i clearlines();

    void clear();
};

#endif