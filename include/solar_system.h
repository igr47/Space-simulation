#ifndef SOLAR_SYSTEM_H
#define SOLAR_SYSTEM_H

#include <vector>
#include <string>
#include "celestialBody.h"

class SolarSystem {
public:
    SolarSystem();
    ~SolarSystem();
    
    void initialize();
    void update(float dt);
    void render();
    void renderSimpleTest();
    
    // User interaction
    void togglePause() { paused = !paused; }
    void setTimeScale(float scale) { timeScale = scale; }
    void resetSimulation();
    void setFollowBody(int index) { followBodyIndex = index; }
    int getFollowBodyIndex() const { return followBodyIndex; }
    const std::vector<CelestialBody>& getBodies() const { return bodies; }
    const std::string& getStatus() const { return statusText; }
    
    // Camera control
    void setCameraDistance(float distance) { cameraDistance = distance; }
    void setCameraAngle(float angle) { cameraAngle = angle; }

    bool getPaused() const { return paused; }
    float getTimeScale() const { return timeScale; }
    float getCameraDistance() const { return cameraDistance; }
    float getCameraAngle() const { return cameraAngle; }
    
private:
    std::vector<CelestialBody> bodies;
    unsigned int sphereVAO, sphereVBO;
    unsigned int trailVAO, trailVBO;
    unsigned int sphereEBO;
    unsigned int sphereIndexCount;
    unsigned int shaderProgram;
    unsigned int trailShaderProgram;
    
    bool paused;
    float timeScale;
    int followBodyIndex;
    float cameraDistance;
    float cameraAngle;
    std::string statusText;
    
    void calculateForces();
    void createSphere(float radius, int sectors, int stacks);
    void createTrailBuffer();
    void renderTrails(const glm::mat4& view, const glm::mat4& projection);
    void checkCollisions();
    
    std::vector<float> sphereVertices;
    
    // Shader methods
    unsigned int compileShader(const char* source, unsigned int type);
    unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource);
    
    // Accurate solar system data
    void initializeRealSolarSystem();
};

#endif
