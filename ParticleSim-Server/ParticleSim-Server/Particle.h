#ifndef PARTICLE_H
#define PARTICLE_H

#include <cmath>
#include <random>
#include <ctime>
#include <iostream>
#include <algorithm>

#ifndef PI
#define PI 3.14159265358979323846
#endif

class Particle {
private:
    static const int size = 3;
    int x, y; // coordinates
    double preciseX, preciseY; // for lower velocities
    int map_x, map_y;
    double velocity;
    double theta;
    static const int SCREEN_WIDTH = 1280;
    static const int SCREEN_HEIGHT = 720;

public:
    Particle(int x, int y, double velocity, double theta);
    void update(double deltaTime);
    int getX() const;
    int getY() const;
    double getTheta() const;
    double getVelocity() const;
    int getSize() const;
    double degreesToRadians(double degrees); // Function declaration
};

#endif // PARTICLE_H