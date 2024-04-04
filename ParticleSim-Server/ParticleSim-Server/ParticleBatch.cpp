#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>
#include <string>


#include "Particle.h"
#include "ParticleBatch.h"

ParticleBatch::ParticleBatch() : numParticles(0) {
    // Start the update thread
    std::cout << "CREATED";
}

std::vector<Particle>& ParticleBatch::getParticles() {
    return particles;
}

int ParticleBatch::getNumParticles() const {
    return particles.size();
}

bool ParticleBatch::isFull() const {
    return numParticles == MAX_LOAD;
}

void ParticleBatch::addNewParticles(const std::vector<Particle>& newParticles, int newN) {
    particles.insert(particles.end(), newParticles.begin(), newParticles.end());
    numParticles += newN;
}

void ParticleBatch::clearParticles() {
    particles.clear();
    numParticles = 0;
}

void ParticleBatch::updateParticles(double timeStep) {
    for (Particle& particle : particles) {
        particle.update(timeStep);
    }
}

