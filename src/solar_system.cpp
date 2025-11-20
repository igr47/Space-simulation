#include "solar_system.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

const float G = 6.67430e-11f; // Gravitational constant (m³ kg⁻¹ s⁻²)

SolarSystem::SolarSystem() 
    : paused(false), timeScale(1.0e6f), followBodyIndex(-1), 
      cameraDistance(15.0f), cameraAngle(0.0f), sphereIndexCount(0) {
    initialize();
}

SolarSystem::~SolarSystem() {
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    glDeleteVertexArrays(1, &trailVAO);
    glDeleteBuffers(1, &trailVBO);
}

void SolarSystem::initialize() {
    initializeRealSolarSystem();
    createSphere(1.0f, 36, 18);
    createTrailBuffer();
    
    // Simple shader source code
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform vec3 color;
        
        out vec3 fragColor;
        
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            fragColor = color;
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 fragColor;
        out vec4 FragColor;
        
        void main() {
            FragColor = vec4(fragColor, 1.0);
        }
    )";
    
    shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    
    // Trail shader
    const char* trailVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        
        uniform mat4 view;
        uniform mat4 projection;
        uniform vec3 color;
        
        out vec3 trailColor;
        
        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
            trailColor = color;
        }
    )";
    
    const char* trailFragmentShader = R"(
        #version 330 core
        in vec3 trailColor;
        out vec4 FragColor;
        
        void main() {
            FragColor = vec4(trailColor, 0.5);
        }
    )";
    
    trailShaderProgram = createShaderProgram(trailVertexShader, trailFragmentShader);
    
    std::cout << "SolarSystem initialized - Shader programs: " << shaderProgram << ", " << trailShaderProgram << std::endl;
}

void SolarSystem::initializeRealSolarSystem() {
    // Clear any existing bodies
    bodies.clear();

    // SIMPLE, STABLE VALUES for testing
    // Using much smaller, manageable numbers

    // Sun (at center) - YELLOW
    bodies.push_back(CelestialBody("Sun", 1.0f, 1.0f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 0.0f)));  // Bright yellow

    // Mercury - GRAY
    bodies.push_back(CelestialBody("Mercury", 0.1f, 0.2f,
        glm::vec3(3.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.6f, 0.6f, 0.6f)));  // Gray

    // Venus - ORANGE
    bodies.push_back(CelestialBody("Venus", 0.2f, 0.3f,
        glm::vec3(6.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.5f, 0.0f)));  // Orange

    // Earth - BLUE
    bodies.push_back(CelestialBody("Earth", 0.2f, 0.3f,
        glm::vec3(9.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.3f, 1.0f)));  // Blue

    // Mars - RED
    bodies.push_back(CelestialBody("Mars", 0.15f, 0.25f,
        glm::vec3(12.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.2f, 0.2f)));  // Red

    std::cout << "Simple solar system initialized with " << bodies.size() << " bodies" << std::endl;

    // Debug: print all positions
    for (const auto& body : bodies) {
        glm::vec3 pos = body.getPosition();
        std::cout << body.getName() << " position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    }
}

/*void SolarSystem::initializeRealSolarSystem() {
    // Clear any existing bodies
    bodies.clear();

    // SIMPLE, STABLE VALUES for testing
    // Using much smaller, manageable numbers

    // Sun (at center)
    bodies.push_back(CelestialBody("Sun", 1.0f, 1.0f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.9f, 0.1f)));

    // Mercury
    bodies.push_back(CelestialBody("Mercury", 0.1f, 0.2f,
        glm::vec3(3.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.5f, 0.0f),
        glm::vec3(0.7f, 0.7f, 0.7f)));

    // Venus
    bodies.push_back(CelestialBody("Venus", 0.2f, 0.3f,
        glm::vec3(5.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.2f, 0.0f),
        glm::vec3(0.9f, 0.7f, 0.3f)));

    // Earth
    bodies.push_back(CelestialBody("Earth", 0.2f, 0.3f,
        glm::vec3(7.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.2f, 0.4f, 1.0f)));

    // Mars
    bodies.push_back(CelestialBody("Mars", 0.15f, 0.25f,
        glm::vec3(10.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.8f, 0.0f),
        glm::vec3(0.8f, 0.3f, 0.2f)));

    std::cout << "Simple solar system initialized with " << bodies.size() << " bodies" << std::endl;

    // Debug: print all positions
    for (const auto& body : bodies) {
        glm::vec3 pos = body.getPosition();
        std::cout << body.getName() << " position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    }
}*/

void SolarSystem::update(float dt) {
    if (paused) return;

    // Simple circular motion for testing - ensure planets move in XY plane
    static float angle = 0.0f;
    angle += dt * 0.8f;  // Faster rotation for better visibility

    // Keep Sun at center
    // Move planets in circular orbits around Sun
    for (size_t i = 1; i < bodies.size(); ++i) { // Start from 1 to skip Sun
        float orbitRadius = 2.0f + (i-1) * 2.5f;  // Increasing orbit radii
        float orbitSpeed = 1.0f / (orbitRadius * 0.5f);  // Slower for outer planets

        bodies[i].position.x = orbitRadius * cos(angle * orbitSpeed + i);
        bodies[i].position.y = 0.0f;  // Keep in XY plane for now
        bodies[i].position.z = orbitRadius * sin(angle * orbitSpeed + i);

        // Update trails
        bodies[i].updateTrail();
    }

    // Update status text
    std::stringstream status;
    status << "Time Scale: " << std::scientific << std::setprecision(1) << timeScale;
    status << " | " << (paused ? "PAUSED" : "RUNNING");
    if (followBodyIndex >= 0 && followBodyIndex < bodies.size()) {
        status << " | Following: " << bodies[followBodyIndex].getName();
    }
    statusText = status.str();
}

/*void SolarSystem::update(float dt) {
    if (paused) return;
    
    float scaledDt = dt * timeScale;
    calculateForces();
    checkCollisions();
    
    for (auto& body : bodies) {
        body.updatePosition(scaledDt);
    }
    
    // Update status text
    std::stringstream status;
    status << "Time Scale: " << std::scientific << std::setprecision(1) << timeScale;
    status << " | " << (paused ? "PAUSED" : "RUNNING");
    if (followBodyIndex >= 0 && followBodyIndex < bodies.size()) {
        status << " | Following: " << bodies[followBodyIndex].getName();
    }
    statusText = status.str();
}*/

void SolarSystem::calculateForces() {
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            glm::vec3 r = bodies[j].getPosition() - bodies[i].getPosition();
            float distance = glm::length(r);
            
            // Avoid division by zero
            if (distance < 1.0f) distance = 1.0f;
            
            float forceMagnitude = G * bodies[i].getMass() * bodies[j].getMass() / (distance * distance);
            glm::vec3 forceDir = glm::normalize(r);
            glm::vec3 force = forceMagnitude * forceDir;
            
            bodies[i].applyForce(force);
            bodies[j].applyForce(-force);
        }
    }
}

void SolarSystem::checkCollisions() {
    // Simple collision detection - can be expanded later
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            if (bodies[i].checkCollision(bodies[j])) {
                std::cout << "COLLISION: " << bodies[i].getName() << " and " << bodies[j].getName() << std::endl;
            }
        }
    }
}

void SolarSystem::renderSimpleTest() {
    // Simple immediate mode rendering to verify basic OpenGL works
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, 1200.0f/800.0f, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 0, 10, 0, 0, 0, 0, 1, 0);

    // Draw a simple colored triangle
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
}

void SolarSystem::render() {
    // Setup camera with better view of the entire system
    glm::vec3 cameraPos, cameraTarget;

    if (followBodyIndex >= 0 && followBodyIndex < bodies.size()) {
        cameraTarget = bodies[followBodyIndex].getPosition();
        cameraPos = cameraTarget + glm::vec3(
            cameraDistance * cos(cameraAngle),
            cameraDistance * 0.3f,
            cameraDistance * sin(cameraAngle)
        );
    } else {
        // Free camera - positioned to see the entire solar system
        cameraTarget = glm::vec3(5.0f, 0.0f, 0.0f); // Look at the middle of the orbits
        cameraPos = glm::vec3(
            15.0f,  // Much closer camera distance
            8.0f,   // Higher viewpoint to see orbits
            0.0f
        );
    }

    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), 1200.0f / 800.0f, 0.1f, 100.0f);

    // Render planets
    if (shaderProgram == 0) {
        std::cout << "Shader program not initialized!" << std::endl;
        return;
    }

    glUseProgram(shaderProgram);

    // Get uniform locations
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(sphereVAO);

    for (const auto& body : bodies) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, body.getPosition());

        // Scale for visibility - make planets much larger relative to their orbits
        float scale;
        if (body.getName() == "Sun") {
            scale = 0.8f;  // Larger Sun
        } else {
            scale = 0.3f;  // Much larger planets so they're visible
        }
        model = glm::scale(model, glm::vec3(scale));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glm::vec3 color = body.getColor();
        glUniform3f(colorLoc, color.r, color.g, color.b);

        glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);

        // Debug: print planet positions occasionally
        static int frameCount = 0;
        if (frameCount++ % 180 == 0) {
            glm::vec3 pos = body.getPosition();
            std::cout << "Rendering " << body.getName() << " at: ("
                      << pos.x << ", " << pos.y << ", " << pos.z
                      << ") with scale: " << scale << std::endl;
        }
    }

    glBindVertexArray(0);

    // Render trails to see planet orbits
    renderTrails(view, projection);
}

/*void SolarSystem::render() {
    // Setup camera
    glm::vec3 cameraPos, cameraTarget;

    if (followBodyIndex >= 0 && followBodyIndex < bodies.size()) {
        cameraTarget = bodies[followBodyIndex].getPosition(); // Already scaled
        cameraPos = cameraTarget + glm::vec3(
            cameraDistance * cos(cameraAngle),
            cameraDistance * 0.5f,
            cameraDistance * sin(cameraAngle)
        );
    } else {
        // Free camera
        cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        cameraPos = glm::vec3(
            cameraDistance * cos(cameraAngle),
            cameraDistance * 0.5f,
            cameraDistance * sin(cameraAngle)
        );
    }

    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), 1200.0f / 800.0f, 0.1f, 1000.0f);

    // Render planets
    if (shaderProgram == 0) {
        std::cout << "Shader program not initialized!" << std::endl;
        return;
    }

    glUseProgram(shaderProgram);

    // Get uniform locations
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(sphereVAO);

    for (const auto& body : bodies) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, body.getPosition()); // Use scaled position directly

        // Scale for visibility
        float scale;
        if (body.getName() == "Sun") {
            scale = 0.2f; // Sun size
        } else {
            scale = 0.05f; // Planet size
        }
        model = glm::scale(model, glm::vec3(scale));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glm::vec3 color = body.getColor();
        glUniform3f(colorLoc, color.r, color.g, color.b);

        glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);

        // Debug output
        static int debugFrame = 0;
        if (debugFrame++ % 120 == 0 && body.getName() == "Sun") {
            glm::vec3 pos = body.getPosition();
            std::cout << "Rendering " << body.getName() << " at: ("
                      << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
        }
    }

    glBindVertexArray(0);

    // Render trails
    renderTrails(view, projection);
}*/

/*void SolarSystem::render() {
    renderSimpleTest();
    return;
    // Setup camera with better parameters
    glm::vec3 cameraPos, cameraTarget;

    if (followBodyIndex >= 0 && followBodyIndex < bodies.size()) {
        cameraTarget = bodies[followBodyIndex].getPosition();
        // Scale down the position for better viewing
        cameraTarget = cameraTarget / 1.0e11f;
        cameraPos = cameraTarget + glm::vec3(
            cameraDistance * cos(cameraAngle),
            cameraDistance * 0.5f,  // Increased height for better view
            cameraDistance * sin(cameraAngle)
        );
    } else {
        // Free camera - look at scaled-down center
        cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        cameraPos = glm::vec3(
            cameraDistance * cos(cameraAngle),
            cameraDistance * 0.5f,
            cameraDistance * sin(cameraAngle)
        );
    }

    // Use more reasonable near/far planes
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), 1200.0f / 800.0f, 0.1f, 1000.0f);

    // Debug camera info
    static int frameCount = 0;
    if (frameCount++ % 60 == 0) {
        std::cout << "Camera - Pos: (" << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z
                  << ") Target: (" << cameraTarget.x << ", " << cameraTarget.y << ", " << cameraTarget.z
                  << ") Distance: " << cameraDistance << std::endl;
    }

    // Render planets first
    if (shaderProgram == 0) {
        std::cout << "Shader program not initialized!" << std::endl;
        return;
    }

    glUseProgram(shaderProgram);

    // Get uniform locations
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");

    if (modelLoc == -1 || viewLoc == -1 || projectionLoc == -1 || colorLoc == -1) {
        std::cout << "Failed to get shader uniform locations!" << std::endl;
        return;
    }

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(sphereVAO);

    for (const auto& body : bodies) {
        glm::mat4 model = glm::mat4(1.0f);

        // Scale down positions for reasonable viewing
        glm::vec3 scaledPosition = body.getPosition() / 1.0e11f;
        model = glm::translate(model, scaledPosition);

        // Scale for visibility - use much larger scales so we can see something
        float scale;
        if (body.getName() == "Sun") {
            scale = 0.5f; // Visible sun size
        } else {
            scale = 0.1f; // Visible planet size
        }
        model = glm::scale(model, glm::vec3(scale));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glm::vec3 color = body.getColor();
        glUniform3f(colorLoc, color.r, color.g, color.b);

        glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);

        // Debug: print first planet position occasionally
        static bool printed = false;
        if (!printed && body.getName() == "Sun") {
            std::cout << "Rendering " << body.getName() << " at position: ("
                      << scaledPosition.x << ", " << scaledPosition.y << ", " << scaledPosition.z
                      << ") with scale: " << scale << std::endl;
            printed = true;
        }
    }

    glBindVertexArray(0);

    // Render trails after planets
    renderTrails(view, projection);
}*/

/*void SolarSystem::render() {
    // Setup camera
    glm::vec3 cameraPos, cameraTarget;
    
    if (followBodyIndex >= 0 && followBodyIndex < bodies.size()) {
        cameraTarget = bodies[followBodyIndex].getPosition();
        cameraPos = cameraTarget + glm::vec3(
            cameraDistance * cos(cameraAngle),
            cameraDistance * 0.3f,
            cameraDistance * sin(cameraAngle)
        );
    } else {
        // Free camera
        cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        cameraPos = glm::vec3(
            cameraDistance * cos(cameraAngle),
            cameraDistance * 0.3f,
            cameraDistance * sin(cameraAngle)
        );
    }
    
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1200.0f / 800.0f, 1e10f, 1e14f);
    
    // Render trails first
    renderTrails(view, projection);
    
    // Render planets
    if (shaderProgram == 0) {
        std::cout << "Shader program not initialized!" << std::endl;
        return;
    }
    
    glUseProgram(shaderProgram);
    
    // Get uniform locations
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    
    if (modelLoc == -1 || viewLoc == -1 || projectionLoc == -1 || colorLoc == -1) {
        std::cout << "Failed to get shader uniform locations!" << std::endl;
        return;
    }
    
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    glBindVertexArray(sphereVAO);
    
    for (const auto& body : bodies) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, body.getPosition());
        
        // Scale for visibility (actual solar system scales are too extreme to see)
        float scale;
        if (body.getName() == "Sun") {
            scale = 5.0e9f; // Sun scale
        } else {
            scale = 1.0e9f; // Planet scale
        }
        model = glm::scale(model, glm::vec3(scale));
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glm::vec3 color = body.getColor();
        glUniform3f(colorLoc, color.r, color.g, color.b);
        
        glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
    }
    
    glBindVertexArray(0);
}*/

/*void SolarSystem::renderTrails(const glm::mat4& view, const glm::mat4& projection) {
    if (trailShaderProgram == 0) return;
    
    glUseProgram(trailShaderProgram);
    
    GLint viewLoc = glGetUniformLocation(trailShaderProgram, "view");
    GLint projectionLoc = glGetUniformLocation(trailShaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(trailShaderProgram, "color");
    
    if (viewLoc != -1 && projectionLoc != -1 && colorLoc != -1) {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        glBindVertexArray(trailVAO);
        
        for (const auto& body : bodies) {
            if (body.getName() == "Sun") continue; // Don't render trail for sun
            
            const auto& trail = body.getTrail();
            if (trail.size() < 2) continue;
            
            // Update trail VBO
            glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
            glBufferData(GL_ARRAY_BUFFER, trail.size() * sizeof(glm::vec3), trail.data(), GL_DYNAMIC_DRAW);
            
            glm::vec3 color = body.getColor();
            glUniform3f(colorLoc, color.r, color.g, color.b);
            
            glDrawArrays(GL_LINE_STRIP, 0, trail.size());
        }
        
        glBindVertexArray(0);
    }
}*/

void SolarSystem::renderTrails(const glm::mat4& view, const glm::mat4& projection) {
    if (trailShaderProgram == 0) return;

    glUseProgram(trailShaderProgram);

    GLint viewLoc = glGetUniformLocation(trailShaderProgram, "view");
    GLint projectionLoc = glGetUniformLocation(trailShaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(trailShaderProgram, "color");

    if (viewLoc != -1 && projectionLoc != -1 && colorLoc != -1) {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(trailVAO);

        for (const auto& body : bodies) {
            if (body.getName() == "Sun") continue;

            const auto& trail = body.getTrail();
            if (trail.size() < 2) continue;

            // Use trail positions directly (already scaled)
            glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
            glBufferData(GL_ARRAY_BUFFER, trail.size() * sizeof(glm::vec3), trail.data(), GL_DYNAMIC_DRAW);

            glm::vec3 color = body.getColor();
            glUniform3f(colorLoc, color.r, color.g, color.b);

            glDrawArrays(GL_LINE_STRIP, 0, trail.size());
        }

        glBindVertexArray(0);
    }
}

void SolarSystem::createSphere(float radius, int sectors, int stacks) {
    sphereVertices.clear();
    std::vector<unsigned int> indices;
    
    float sectorStep = 2 * M_PI / sectors;
    float stackStep = M_PI / stacks;
    
    // Generate vertices
    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = M_PI / 2 - i * stackStep;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);
        
        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * sectorStep;
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);
            
            sphereVertices.push_back(x);
            sphereVertices.push_back(y);
            sphereVertices.push_back(z);
        }
    }
    
    // Generate indices
    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;
        
        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != (stacks - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
    
    sphereIndexCount = indices.size();
    
    // Create VAO/VBO/EBO
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);
    
    glBindVertexArray(sphereVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    std::cout << "Sphere created with " << sphereVertices.size() / 3 << " vertices and " << sphereIndexCount << " indices" << std::endl;
}

void SolarSystem::createTrailBuffer() {
    glGenVertexArrays(1, &trailVAO);
    glGenBuffers(1, &trailVBO);
    
    glBindVertexArray(trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
    
    // Initialize with empty data
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void SolarSystem::resetSimulation() {
    bodies.clear();
    initializeRealSolarSystem();
}

unsigned int SolarSystem::compileShader(const char* source, unsigned int type) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED: " << infoLog << std::endl;
        std::cout << "Shader type: " << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT") << std::endl;
    } else {
        std::cout << "Shader compiled successfully: " << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT") << std::endl;
    }
    return shader;
}

unsigned int SolarSystem::createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    unsigned int vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);
    
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED: " << infoLog << std::endl;
        program = 0;
    } else {
        std::cout << "Shader program linked successfully: " << program << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}
