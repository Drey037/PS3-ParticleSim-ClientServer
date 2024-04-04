#include "Particle.h"

Particle::Particle(int x, int y, double velocity, double theta) {
    this->x = x;
    this->y = y;
    this->velocity = velocity;
    this->theta = theta;
    this->map_x = x * 4;
    this->map_y = y * 4;
    this->preciseX = x;
    this->preciseY = y;
}

void Particle::update(double deltaTime) {
    double radians = degreesToRadians(theta);
    preciseX += (velocity * std::cos(radians)) * deltaTime;
    preciseY += (velocity * std::sin(radians)) * deltaTime;
    this->x = static_cast<int>(std::round(preciseX));
    this->y = static_cast<int>(std::round(preciseY));

    if (x <= 0 || x >= SCREEN_WIDTH) {
        theta = 180 - theta;
        x = std::max(0, std::min(x, SCREEN_WIDTH));
    }
    if (y <= 0 || y >= SCREEN_HEIGHT) {
        theta = -theta;
        y = std::max(0, std::min(y, SCREEN_HEIGHT));
    }
}

int Particle::getX() const {
    return x;
}

int Particle::getY() const {
    return y;
}

int Particle::getSize() const {
    return size;
}

double Particle::degreesToRadians(double degrees) {
    return degrees * PI / 180.0;
}