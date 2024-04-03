#ifndef PARTICLE_H
#define PARTICLE_H

#include <cmath>
#include <random>
#include <ctime>

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
    bool isExplorerMode;
    static std::mt19937 randomEngine; // for generating random color

public:
    Particle(int x, int y, double velocity, double theta) {
        this->x = x;
        this->y = y;
        this->velocity = velocity;
        this->theta = std::toRadians(theta);
        isExplorerMode = false;
        this->map_x = x * 4;
        this->map_y = y * 4;
        this->preciseX = x;
        this->preciseY = y;
    }

    void update(double deltaTime) {
        double deltaTimeSeconds = deltaTime / 1000.0;
        preciseX += (velocity * std::cos(this->theta)) * deltaTimeSeconds;
        preciseY += (velocity * std::sin(this->theta)) * deltaTimeSeconds;
        this->x = static_cast<int>(std::round(preciseX));
        this->y = static_cast<int>(std::round(preciseY));
        if (x <= 0 || x >= SCREEN_WIDTH) {
            theta = M_PI - theta; // Reflect horizontally
            x = std::max(0, std::min(x, SCREEN_WIDTH)); // Keep within bounds
        }
        if (y <= 0 || y >= SCREEN_HEIGHT) {
            theta = -theta; // Reflect vertically
            y = std::max(0, std::min(y, SCREEN_HEIGHT)); // Keep within bounds
        }
    }

    int getX() const {
        return x;
    }

    int getY() const {
        return y;
    }

    int getSize() const {
        return size;
    }
};

#endif // PARTICLE_H