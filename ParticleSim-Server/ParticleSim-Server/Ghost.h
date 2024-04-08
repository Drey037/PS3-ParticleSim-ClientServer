#ifndef GHOST_H
#define GHOST_H

#include <iostream>
#include <string>
#include <winsock2.h>

class Ghost {
public:
    Ghost(int x, int y, SOCKET socket);
    void turnChar(bool isLeft);
    SOCKET getSocket();
    int getX() const; // Declare as const
    int getY() const; // Declare as const
    int getID() const; 
    static int currentID;

    // Add setX and setY methods
    void setX(int newX);
    void setY(int newY);

private:
    int x, y;
    bool isLeft;
    int id;
    SOCKET socket; // Assuming Socket is a pointer type
};

#endif // GHOST_H