#include "Ghost.h"

Ghost::Ghost(int x, int y, SOCKET socket)
    : x(x), y(y), isLeft(true), socket(socket), id(++currentID) {}

void Ghost::turnChar(bool isLeft) {
    this->isLeft = isLeft;
}

SOCKET Ghost::getSocket() {
    if (socket == INVALID_SOCKET) {
        // Handle the error, e.g., throw an exception or return a default value
        throw std::runtime_error("Socket is not initialized");
    }
    return socket;
}

int Ghost::getX() {
    return x;
}

int Ghost::getY() {
    return y;
}

int Ghost::getID() const {
    return id;
}