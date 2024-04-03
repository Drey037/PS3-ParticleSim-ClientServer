#ifndef PARTICLEBATCH_H
#define PARTICLEBATCH_H

#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>

#include "Particle.h"

class ParticleBatch {
private:
    std::vector<Particle> particles;
    std::mutex particleListLock;
    int numParticles;
    static const int MAX_LOAD = 10;

public:
    ParticleBatch() : numParticles(0) {}

    std::vector<Particle>& getParticles() {
        return particles;
    }

    int getNumParticles() const {
        return particles.size();
    }

    bool isFull() const {
        return particles.size() == MAX_LOAD;
    }

    void addNewParticles(const std::vector<Particle>& newParticles) {
        particles.insert(particles.end(), newParticles.begin(), newParticles.end());
        numParticles += newParticles.size();
    }

    void clearParticles() {
        particles.clear();
        numParticles = 0;
    }

    void run() {
        auto lastUpdateTime = std::chrono::steady_clock::now();

        while (true) {
            {
                std::lock_guard<std::mutex> lock(particleListLock);
                auto currentTime = std::chrono::steady_clock::now();
                auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastUpdateTime).count();
                lastUpdateTime = currentTime;

                for (Particle& particle : particles) {
                    particle.update(deltaTime);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
};

#endif // PARTICLEBATCH_H