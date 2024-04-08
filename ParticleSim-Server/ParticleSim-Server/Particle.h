#ifndef PARTICLE_H
#define PARTICLE_H

#include <cmath>
#include <random>
#include <ctime>
#include <iostream>
#include <algorithm>

class Particle {
private:
    static const int size = 2;
    int x, y; // coordinates
    double preciseX, preciseY; // for lower velocities
    int map_x, map_y;
    double velocity;
    double theta;
    static const int SCREEN_WIDTH = 1280;
    static const int SCREEN_HEIGHT = 720;
    static const double PI; // Ensure PI is defined and accessible

public:
    Particle(int x, int y, double velocity, double theta);
    void update(double deltaTime);
    int getX() const;
    int getY() const;
    double getTheta() const;
    double getVelocity() const;
    int getSize() const;
    static double degreesToRadians(double degrees); // Function declaration
};

#endif // PARTICLE_H