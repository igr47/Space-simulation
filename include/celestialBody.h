#ifndef CELESTIAL_BODY_H
#define CELESTIAL_BODY_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

class CelestialBody {
public:
    CelestialBody(const std::string& name, float mass, float radius, 
                  glm::vec3 position, glm::vec3 velocity, glm::vec3 color,
                  float orbitalPeriod = 0.0f);
    
    void updatePosition(float dt);
    void applyForce(glm::vec3 force);
    void resetForce();
    void updateTrail();
    bool checkCollision(const CelestialBody& other) const;
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    float getMass() const { return mass; }
    float getRadius() const { return radius; }
    glm::vec3 getColor() const { return color; }
    std::string getName() const { return name; }
    const std::vector<glm::vec3>& getTrail() const { return trail; }
    float getOrbitalPeriod() const { return orbitalPeriod; }
    glm::vec3 position;
    
private:
    std::string name;
    float mass;
    float radius;
    //glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 force;
    glm::vec3 color;
    float orbitalPeriod;
    
    std::vector<glm::vec3> trail;
    const size_t maxTrailPoints = 500;
};

#endif
