#ifndef PARTICLEBATCH_H
#define PARTICLEBATCH_H

#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>


class ParticleBatch {
private:
    std::vector<Particle> particles;
    std::mutex particleListLock;
    int numParticles;
    static const int MAX_LOAD = 20;
    std::thread updateThread; // Thread for running the update loop

public:
    ParticleBatch();

    std::vector<Particle>& getParticles();
    int getNumParticles() const;
    bool isFull() const;
    void addNewParticles(const std::vector<Particle>& newParticles, int newN);
    void clearParticles();
    void updateParticles(double timeStep);

    //Delete copy constructor
    ParticleBatch(const ParticleBatch&) = delete;

   //Delete copy assignment operator
    ParticleBatch& operator=(const ParticleBatch&) = delete;

};

#endif // PARTICLEBATCH_H