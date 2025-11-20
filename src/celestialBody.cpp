#include "celestialBody.h"
#include <cmath>

CelestialBody::CelestialBody(const std::string& name, float mass, float radius, 
                           glm::vec3 position, glm::vec3 velocity, glm::vec3 color,
                           float orbitalPeriod)
    : name(name), mass(mass), radius(radius), position(position), 
      velocity(velocity), color(color), orbitalPeriod(orbitalPeriod) {
    acceleration = glm::vec3(0.0f);
    force = glm::vec3(0.0f);
    trail.reserve(maxTrailPoints);
}

void CelestialBody::updatePosition(float dt) {
    if (mass == 0.0f) return; // Static body (like sun in simplified mode)
    
    acceleration = force / mass;
    velocity += acceleration * dt;
    position += velocity * dt;
    resetForce();
    
    updateTrail();
}

void CelestialBody::applyForce(glm::vec3 newForce) {
    force += newForce;
}

void CelestialBody::resetForce() {
    force = glm::vec3(0.0f);
}

void CelestialBody::updateTrail() {
    trail.push_back(position);
    if (trail.size() > maxTrailPoints) {
        trail.erase(trail.begin());
    }
}

bool CelestialBody::checkCollision(const CelestialBody& other) const {
    float distance = glm::length(position - other.position);
    return distance < (radius + other.radius);
}
