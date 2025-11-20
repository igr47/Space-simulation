#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string>
#include "solar_system.h"

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// Global variables
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

SolarSystem* solarSystemPtr = nullptr;
bool mousePressed = false;
double lastX = SCR_WIDTH / 2.0;
double lastY = SCR_HEIGHT / 2.0;

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Set OpenGL version to 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Advanced Solar System Simulation", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    // Set callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    
    // Initialize GLEW with experimental mode
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        std::cout << "Failed to initialize GLEW: " << glewGetErrorString(glewError) << std::endl;
        
        // Try to continue anyway - sometimes GLEW fails but OpenGL still works
        std::cout << "Attempting to continue without GLEW..." << std::endl;
    }
    
    // Print OpenGL version info
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    
    // Check for error
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "OpenGL error: " << err << std::endl;
    }
    
    // Enable features
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Clear any initial OpenGL errors (common with GLEW experimental)
    while (glGetError() != GL_NO_ERROR);
    
    SolarSystem solarSystem;
    solarSystemPtr = &solarSystem;
    glfwSetWindowUserPointer(window, &solarSystem);
    
    float lastTime = glfwGetTime();
    
    std::cout << "\nControls:\n";
    std::cout << "SPACE - Pause/Resume simulation\n";
    std::cout << "R - Reset simulation\n";
    std::cout << "UP/DOWN - Increase/Decrease time scale\n";
    std::cout << "1-7 - Follow specific planet (1=Sun, 2=Mercury, etc.)\n";
    std::cout << "0 - Free camera mode\n";
    std::cout << "Mouse Scroll - Zoom in/out\n";
    std::cout << "Mouse Drag - Rotate camera\n";
    std::cout << "ESC - Exit\n\n";
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        // Process input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Update simulation
        solarSystem.update(deltaTime);
        
        // Render
        solarSystem.render();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    SolarSystem* solarSystem = static_cast<SolarSystem*>(glfwGetWindowUserPointer(window));
    if (!solarSystem) return;

    switch (key) {
        case GLFW_KEY_SPACE:
            solarSystem->togglePause();
            std::cout << (solarSystem->getPaused() ? "Simulation PAUSED" : "Simulation RUNNING") << std::endl;
            break;

        case GLFW_KEY_R:
            solarSystem->resetSimulation();
            std::cout << "Simulation RESET" << std::endl;
            break;

        case GLFW_KEY_UP:
            solarSystem->setTimeScale(solarSystem->getTimeScale() * 2.0f);
            std::cout << "Time scale: " << solarSystem->getTimeScale() << std::endl;
            break;

        case GLFW_KEY_DOWN:
            solarSystem->setTimeScale(solarSystem->getTimeScale() / 2.0f);
            std::cout << "Time scale: " << solarSystem->getTimeScale() << std::endl;
            break;

        case GLFW_KEY_0:
            solarSystem->setFollowBody(-1);
            std::cout << "Camera: Free mode" << std::endl;
            break;

        case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3: case GLFW_KEY_4:
        case GLFW_KEY_5:
            {
                int bodyIndex = key - GLFW_KEY_1;
                solarSystem->setFollowBody(bodyIndex);
                auto bodies = solarSystem->getBodies();
                if (bodyIndex < bodies.size()) {
                    std::cout << "Now following: " << bodies[bodyIndex].getName() << std::endl;
                }
            }
            break;

        // Add camera controls
        case GLFW_KEY_W:
            solarSystem->setCameraDistance(solarSystem->getCameraDistance() * 0.9f);
            std::cout << "Camera zoomed in" << std::endl;
            break;

        case GLFW_KEY_S:
            solarSystem->setCameraDistance(solarSystem->getCameraDistance() * 1.1f);
            std::cout << "Camera zoomed out" << std::endl;
            break;

        case GLFW_KEY_A:
            solarSystem->setCameraAngle(solarSystem->getCameraAngle() - 0.2f);
            std::cout << "Camera rotated left" << std::endl;
            break;

        case GLFW_KEY_D:
            solarSystem->setCameraAngle(solarSystem->getCameraAngle() + 0.2f);
            std::cout << "Camera rotated right" << std::endl;
            break;
    }
}

/*void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    
    SolarSystem* solarSystem = static_cast<SolarSystem*>(glfwGetWindowUserPointer(window));
    if (!solarSystem) return;
    
    switch (key) {
        case GLFW_KEY_SPACE:
            solarSystem->togglePause();
            std::cout << (solarSystem->getPaused() ? "Simulation PAUSED" : "Simulation RUNNING") << std::endl;
            break;
            
        case GLFW_KEY_R:
            solarSystem->resetSimulation();
            std::cout << "Simulation RESET" << std::endl;
            break;
            
        case GLFW_KEY_UP:
            solarSystem->setTimeScale(solarSystem->getTimeScale() * 2.0f);
            std::cout << "Time scale: " << solarSystem->getTimeScale() << std::endl;
            break;
            
        case GLFW_KEY_DOWN:
            solarSystem->setTimeScale(solarSystem->getTimeScale() / 2.0f);
            std::cout << "Time scale: " << solarSystem->getTimeScale() << std::endl;
            break;
            
        case GLFW_KEY_0:
            solarSystem->setFollowBody(-1);
            std::cout << "Camera: Free mode" << std::endl;
            break;
            
        case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3: case GLFW_KEY_4:
        case GLFW_KEY_5: case GLFW_KEY_6: case GLFW_KEY_7:
            {
                int bodyIndex = key - GLFW_KEY_1;
                solarSystem->setFollowBody(bodyIndex);
                auto bodies = solarSystem->getBodies();
                if (bodyIndex < bodies.size()) {
                    std::cout << "Now following: " << bodies[bodyIndex].getName() << std::endl;
                }
            }
            break;
    }
}*/

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    SolarSystem* solarSystem = static_cast<SolarSystem*>(glfwGetWindowUserPointer(window));
    if (solarSystem) {
        float currentDistance = solarSystem->getCameraDistance();
        float newDistance = currentDistance * (yoffset > 0 ? 0.9f : 1.1f);
        // Limit zoom range
        if (newDistance > 1e10f && newDistance < 1e14f) {
            solarSystem->setCameraDistance(newDistance);
        }
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (mousePressed) {
        SolarSystem* solarSystem = static_cast<SolarSystem*>(glfwGetWindowUserPointer(window));
        if (solarSystem) {
            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top
            
            lastX = xpos;
            lastY = ypos;
            
            float sensitivity = 0.001f;
            xoffset *= sensitivity;
            yoffset *= sensitivity;
            
            float currentAngle = solarSystem->getCameraAngle();
            solarSystem->setCameraAngle(currentAngle + xoffset);
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mousePressed = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        } else {
            mousePressed = false;
        }
    }
}
