#include <glad/glad.h>
#include <ctime>
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <map>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Shader sources with texture support
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 WorldPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    WorldPos = worldPos.xyz;
    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * worldPos;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 WorldPos;

uniform sampler2D diffuseTexture;
uniform sampler2D nightTexture;
uniform int hasNightTexture;
uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float ambientStrength;
uniform float specularStrength;
uniform int isEmissive;
uniform float emissiveStrength;
uniform float roughness;
uniform float isRings;

void main() {
    if (isEmissive == 1) {
        vec3 texColor = texture(diffuseTexture, TexCoord).rgb;
        float dist = length(WorldPos - lightPos);
        float glow = 1.0 / (1.0 + 0.0001 * dist * dist);
        vec3 emission = texColor * emissiveStrength * (1.0 + glow * 2.0);
        FragColor = vec4(emission, 1.0);
        return;
    }

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 texColor = texture(diffuseTexture, TexCoord).rgb;

    if (hasNightTexture == 1 && diff < 0.2) {
        float nightIntensity = 1.0 - (diff / 0.2);
        vec3 nightColor = texture(nightTexture, TexCoord).rgb;
        texColor = mix(texColor, nightColor, nightIntensity * 0.8);
    }

    vec3 ambient = ambientStrength * lightColor;
    vec3 diffuse = diff * lightColor;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0 * (1.0 - roughness));
    vec3 specular = specularStrength * spec * lightColor;
    float dist = length(lightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.0000001 * dist * dist);
    vec3 result = (ambient + (diffuse + specular) * attenuation) * texColor;
    float fresnel = 1.0 - max(dot(viewDir, norm), 0.0);
    fresnel = pow(fresnel, 2.0);
    result += texColor * fresnel * 0.15;
    if (isRings > 0.5) result *= 0.9;
    FragColor = vec4(result, isRings > 0.5 ? 0.8 : 1.0);
}
)";

const char* orbitVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* orbitFragmentShader = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
uniform float alpha;
void main() {
    FragColor = vec4(color, alpha);
}
)";

struct Textures {
    unsigned int diffuse;
    unsigned int night;
};

struct Mesh {
    unsigned int VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    int indexCount;
};

struct Asteroid {
    float orbitRadius;
    float orbitAngle;
    float orbitSpeed;
    float yOffset;
    float radius;
    float rotationAngle;
    float rotationSpeed;
    glm::vec3 rotationAxis;
};

struct Comet {
    glm::vec3 position;
    glm::vec3 velocity;
    float tailLength;
    float life;
    float radius;
    bool is3IAtlas;
};
std::vector<Comet> comets;

Mesh createSphere(float radius, int sectors = 32, int stacks = 32) {
    Mesh mesh;
    std::vector<float> verts;
    std::vector<unsigned int> inds;
    const float PI = 3.14159265359f;
    for (int i = 0; i <= stacks; ++i) {
        float phi = PI * i / stacks;
        for (int j = 0; j <= sectors; ++j) {
            float theta = 2 * PI * j / sectors;
            float x = sin(phi) * cos(theta);
            float y = cos(phi);
            float z = sin(phi) * sin(theta);
            verts.push_back(radius * x);
            verts.push_back(radius * y);
            verts.push_back(radius * z);
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
            verts.push_back((float)j / sectors);
            verts.push_back((float)i / stacks);
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int a = i * (sectors + 1) + j;
            int b = a + sectors + 1;
            inds.push_back(a); inds.push_back(b); inds.push_back(a + 1);
            inds.push_back(b); inds.push_back(b + 1); inds.push_back(a + 1);
        }
    }
    mesh.vertices = verts;
    mesh.indices = inds;
    mesh.indexCount = inds.size();
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(unsigned int), inds.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return mesh;
}

std::vector<Asteroid> createAsteroidBelt(float innerR, float outerR, float height, int numAsteroids) {
    std::vector<Asteroid> asteroids;
    for (int i = 0; i < numAsteroids; ++i) {
        Asteroid a;
        a.orbitAngle = ((float)rand() / RAND_MAX) * 2.0f * 3.14159265f;
        a.orbitRadius = innerR + ((float)rand() / RAND_MAX) * (outerR - innerR);
        a.yOffset = (((float)rand() / RAND_MAX) - 0.5f) * height;
        a.radius = 0.3f + ((float)rand() / RAND_MAX) * 2.0f;
        a.orbitSpeed = 0.02f + ((float)rand() / RAND_MAX) * 0.05f;
        a.rotationAngle = ((float)rand() / RAND_MAX) * 360.0f;
        a.rotationSpeed = 10.0f + ((float)rand() / RAND_MAX) * 90.0f;
        float ra = ((float)rand() / RAND_MAX) * 2.0f * 3.14159265f;
        float dec = ((float)rand() / RAND_MAX) * 3.14159265f;
        a.rotationAxis = glm::vec3(sin(dec)*cos(ra), cos(dec), sin(dec)*sin(ra));
        asteroids.push_back(a);
    }
    return asteroids;
}

Mesh createRing(float innerRadius, float outerRadius, int segments = 64) {
    Mesh mesh;
    std::vector<float> verts;
    std::vector<unsigned int> inds;
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * 3.14159265359f * i / segments;
        float ca = cos(angle), sa = sin(angle);
        verts.push_back(outerRadius * ca); verts.push_back(0.0f); verts.push_back(outerRadius * sa);
        verts.push_back(0.0f); verts.push_back(1.0f); verts.push_back(0.0f);
        verts.push_back((float)i / segments); verts.push_back(1.0f);
        verts.push_back(innerRadius * ca); verts.push_back(0.0f); verts.push_back(innerRadius * sa);
        verts.push_back(0.0f); verts.push_back(1.0f); verts.push_back(0.0f);
        verts.push_back((float)i / segments); verts.push_back(0.0f);
        if (i < segments) {
            int base = i * 2;
            inds.push_back(base); inds.push_back(base + 1); inds.push_back(base + 2);
            inds.push_back(base + 1); inds.push_back(base + 3); inds.push_back(base + 2);
        }
    }
    mesh.vertices = verts; mesh.indices = inds; mesh.indexCount = inds.size();
    glGenVertexArrays(1, &mesh.VAO); glGenBuffers(1, &mesh.VBO); glGenBuffers(1, &mesh.EBO);
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(unsigned int), inds.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    return mesh;
}

unsigned int loadTexture(const char* path, bool flipVertically = true) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    stbi_set_flip_vertically_on_load(flipVertically);
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

unsigned int createWhiteTexture() {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    unsigned char white[4] = {255, 255, 255, 255};
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return textureID;
}

unsigned int createOrbitRing(float radius, int segments = 128) {
    std::vector<float> verts;
    for (int i = 0; i <= segments; ++i) {
        float theta = 2.0f * 3.14159265359f * i / segments;
        verts.push_back(radius * cos(theta));
        verts.push_back(0.0f);
        verts.push_back(radius * sin(theta));
    }
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return VAO;
}

unsigned int compileShader(const char* source, GLenum type) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
    }
    return shader;
}

unsigned int createShaderProgram(const char* vsSource, const char* fsSource) {
    unsigned int vs = compileShader(vsSource, GL_VERTEX_SHADER);
    unsigned int fs = compileShader(fsSource, GL_FRAGMENT_SHADER);
    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Shader linking failed: " << infoLog << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    float yaw = -90.0f;
    float pitch = 20.0f;
    float speed = 1000.0f;
    float mouseSensitivity = 0.1f;
    float fov = 45.0f;

    Camera(glm::vec3 pos = glm::vec3(0.0f, 2000.0f, 5000.0f)) {
        position = pos;
        updateVectors();
    }

    void updateVectors() {
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);
        right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        up = glm::normalize(glm::cross(right, front));
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    void processMouse(float xoffset, float yoffset) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;
        yaw += xoffset;
        pitch += yoffset;
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        updateVectors();
    }

    void processKeyboard(int dir, float deltaTime) {
        float vel = speed * deltaTime;
        if (dir == 0) position += front * vel;
        if (dir == 1) position -= front * vel;
        if (dir == 2) position -= right * vel;
        if (dir == 3) position += right * vel;
        if (dir == 4) position += up * vel;
        if (dir == 5) position -= up * vel;
    }
};

struct Moon {
    std::string name;
    float radius;
    float distance;
    float orbitalPeriod;
    float inclination;
    Textures textures;
    float roughness;
    float currentAngle;
    unsigned int orbitVAO;
};

struct Planet {
    std::string name;
    float radius;
    float distance;
    float orbitalPeriod;
    float rotationPeriod;
    float inclination;
    float axialTilt;
    Textures textures;
    float roughness;
    float specular;
    float currentOrbitAngle;
    float currentRotationAngle;
    unsigned int orbitVAO;
    std::vector<Moon> moons;
    bool hasRings;
    float ringInnerRadius;
    float ringOuterRadius;
};

std::vector<Planet> createSolarSystem() {
    std::vector<Planet> planets;
    auto compressDistance = [](float realAU) -> float {
        float realMkm = realAU * 149.6f;
        if (realMkm < 100) return realMkm * 2.0f;
        return 200.0f + (realMkm - 100.0f) * 0.4f;
    };

    planets.push_back({
        "Mercury", 2.4f, compressDistance(0.39f), 88.0f, 1407.6f, 7.0f, 0.03f,
        {loadTexture("2k_mercury.jpg"), 0}, 0.7f, 0.3f, 0.0f, 0.0f, 0, {}, false, 0, 0
    });

    planets.push_back({
        "Venus", 6.0f, compressDistance(0.72f), 224.7f, 5832.5f, 3.4f, 177.4f,
        {loadTexture("2k_venus_surface.jpg"), loadTexture("2k_venus_atmosphere.jpg")}, 0.5f, 0.4f, 0.0f, 0.0f, 0, {}, false, 0, 0
    });

    Planet earth = {
        "Earth", 6.4f, compressDistance(1.0f), 365.25f, 24.0f, 0.0f, 23.5f,
        {loadTexture("2k_earth_daymap.jpg"), loadTexture("2k_earth_nightmap.jpg")}, 0.4f, 0.5f, 0.0f, 0.0f, 0, {}, false, 0, 0
    };
    earth.moons.push_back({"Moon", 1.7f, 12.0f, 27.3f, 5.1f, 
        {loadTexture("2k_moon.jpg"), 0}, 0.9f, 0.0f, 0});
    planets.push_back(earth);

    Planet mars = {
        "Mars", 3.4f, compressDistance(1.52f), 687.0f, 24.6f, 1.9f, 25.2f,
        {loadTexture("2k_mars.jpg"), 0}, 0.8f, 0.3f, 0.0f, 0.0f, 0, {}, false, 0, 0
    };
    mars.moons.push_back({"Phobos", 0.15f, 0.009f * 1000.0f, 0.32f, 1.0f, 
        {loadTexture("2k_moon.jpg"), 0}, 0.95f, 0.0f, 0});
    mars.moons.push_back({"Deimos", 0.08f, 0.023f * 1000.0f, 1.26f, 2.0f, 
        {loadTexture("2k_moon.jpg"), 0}, 0.95f, 0.0f, 0});
    planets.push_back(mars);

    Planet jupiter = {
        "Jupiter", 71.5f, compressDistance(5.2f), 4333.0f, 9.9f, 1.3f, 3.1f,
        {loadTexture("2k_jupiter.jpg"), 0}, 0.3f, 0.6f, 0.0f, 0.0f, 0, {}, false, 0, 0
    };
    for (int i = 0; i < 4; i++) {
        float distance;
        switch (i) {
            case 0: distance = 15.0f; break; // Io 
            case 1: distance = 22.0f; break; // Europa 
            case 2: distance = 35.0f; break; // Ganmende 
            case 3: distance = 55.0f; break; // Callisto
        }
        jupiter.moons.push_back({"Moon", i == 0 ? 1.8f : (i == 1 ? 1.6f : (i == 2 ? 2.6f : 2.4f)), 
            distance,
            i == 0 ? 1.77f : (i == 1 ? 3.55f : (i == 2 ? 7.15f : 16.69f)), 
            0.0f, {loadTexture("2k_moon.jpg"), 0}, 0.7f, 0.0f, 0});
    }
    planets.push_back(jupiter);

    Planet saturn = {
        "Saturn", 60.3f, compressDistance(9.5f), 10759.0f, 10.7f, 2.5f, 26.7f,
        {loadTexture("2k_saturn.jpg"), 0}, 0.3f, 0.7f, 0.0f, 0.0f, 0, {}, true, 90.0f, 140.0f
    };
    for (int i = 0; i < 4; i++) {
        float distance;
        switch(i) {
            case 0: distance = 20.0f; break;  // Titan
            case 1: distance = 10.0f; break;  // Enceladus
            case 2: distance = 45.0f; break;  // Iapetus
            case 3: distance = 8.0f; break;   // Mimas
        }
        saturn.moons.push_back({"Moon", i == 0 ? 2.6f : (i == 1 ? 1.5f : (i == 2 ? 1.5f : 1.1f)), 
            distance, 
            i == 0 ? 15.95f : (i == 1 ? 4.52f : (i == 2 ? 79.33f : 2.74f)), 
            0.0f, {loadTexture("2k_moon.jpg"), 0}, 0.7f, 0.0f, 0});
    }
    planets.push_back(saturn);

    planets.push_back({
        "Uranus", 25.6f, compressDistance(19.2f), 30687.0f, 17.2f, 0.8f, 97.8f,
        {loadTexture("2k_uranus.jpg"), 0}, 0.5f, 0.5f, 0.0f, 0.0f, 0, {}, false, 0, 0
    });

    planets.push_back({
        "Neptune", 24.8f, compressDistance(30.1f), 60190.0f, 16.1f, 1.8f, 28.3f,
        {loadTexture("2k_neptune.jpg"), 0}, 0.4f, 0.6f, 0.0f, 0.0f, 0, {}, false, 0, 0
    });

    return planets;
}

Camera camera;
bool firstMouse = true;
float lastX = 400, lastY = 300;
bool keys[1024] = {false};
float timeScale = 1.0f;
bool paused = false;
int focusedPlanet = -1;

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos; lastY = ypos; firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    camera.processMouse(xoffset, yoffset);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.fov -= (float)yoffset;
    if (camera.fov < 1.0f) camera.fov = 1.0f;
    if (camera.fov > 90.0f) camera.fov = 90.0f;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        keys[key] = true;
        if (key == GLFW_KEY_SPACE) paused = !paused;
        if (key == GLFW_KEY_1) timeScale = 0.1f;
        if (key == GLFW_KEY_2) timeScale = 1.0f;
        if (key == GLFW_KEY_3) timeScale = 10.0f;
        if (key == GLFW_KEY_4) timeScale = 100.0f;
        if (key == GLFW_KEY_5) timeScale = 500.0f;
        if (key == GLFW_KEY_0) focusedPlanet = -1;
        if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F8) {
            focusedPlanet = key - GLFW_KEY_F1;
            if (focusedPlanet >= 8) focusedPlanet = -1;
        }
    }
    if (action == GLFW_RELEASE) keys[key] = false;
}

void processInput(GLFWwindow* window, float deltaTime) {
    if (keys[GLFW_KEY_W]) camera.processKeyboard(0, deltaTime);
    if (keys[GLFW_KEY_S]) camera.processKeyboard(1, deltaTime);
    if (keys[GLFW_KEY_A]) camera.processKeyboard(2, deltaTime);
    if (keys[GLFW_KEY_D]) camera.processKeyboard(3, deltaTime);
    if (keys[GLFW_KEY_Q]) camera.processKeyboard(4, deltaTime);
    if (keys[GLFW_KEY_E]) camera.processKeyboard(5, deltaTime);
    if (keys[GLFW_KEY_ESCAPE]) glfwSetWindowShouldClose(window, true);
}

void spawn3IAtlas(std::vector<Comet>& comets, float marsOrbitRadius) {
    float angle = 0.6f;
    float dist = marsOrbitRadius * 1.35f;
    float startX = dist * cos(angle);
    float startZ = dist * sin(angle);
    float startY = 120.0f;
    glm::vec3 pos(startX, startY, startZ);
    glm::vec3 toSun = glm::normalize(glm::vec3(0.0f) - pos);
    glm::vec3 tangent(-toSun.z, 0.0f, toSun.x);
    glm::vec3 dir = glm::normalize(toSun + tangent * 0.35f);
    Comet atlas;
    atlas.position = pos;
    atlas.velocity = dir * 1500.0f;
    atlas.tailLength = 500.0f;
    atlas.life = 90.0f;
    atlas.radius = 3.0f;
    atlas.is3IAtlas = true;
    comets.push_back(atlas);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    auto compressDistance = [](float realAU) -> float {
        float realMkm = realAU * 149.6f;
        if (realMkm < 100) return realMkm * 2.0f;
        return 200.0f + (realMkm - 100.0f) * 0.4f;
    };

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1600, 900, "Realistic Solar System Simulation", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    unsigned int planetShader = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    unsigned int orbitShader = createShaderProgram(orbitVertexShader, orbitFragmentShader);

    auto planets = createSolarSystem();
    float marsOrbitRadius = compressDistance(1.52f);

    std::vector<Asteroid> asteroids = createAsteroidBelt(
        compressDistance(1.8f), compressDistance(2.8f), 40.0f, 1200);
    Mesh asteroidMesh = createSphere(1.0f, 5, 5);

    Mesh sunMesh = createSphere(80.0f, 64, 64);
    unsigned int sunTexture = loadTexture("2k_sun.jpg");
    unsigned int whiteTexture = createWhiteTexture();

    std::vector<Mesh> planetMeshes;
    std::vector<Mesh> ringMeshes;
    for (auto& p : planets) {
        int stacks = std::max(24, (int)(p.radius / 2.0f));
        planetMeshes.push_back(createSphere(p.radius, stacks, stacks));
        p.orbitVAO = createOrbitRing(p.distance);
        if (p.hasRings) {
            ringMeshes.push_back(createRing(p.ringInnerRadius, p.ringOuterRadius, 128));
        }
        for (auto& m : p.moons) {
            m.orbitVAO = createOrbitRing(m.distance);
        }
    }

    std::vector<Mesh> moonMeshes;
    for (auto& p : planets) {
        for (auto& m : p.moons) {
            int stacks = std::max(12, (int)(m.radius * 4));
            moonMeshes.push_back(createSphere(m.radius, stacks, stacks));
        }
    }

    Mesh cometHeadMesh = createSphere(1.0f, 8, 8);

    unsigned int cometTailVAO, cometTailVBO;
    glGenVertexArrays(1, &cometTailVAO);
    glGenBuffers(1, &cometTailVBO);
    glBindVertexArray(cometTailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cometTailVBO);
    glBufferData(GL_ARRAY_BUFFER, 20 * 6 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    std::vector<float> starVerts;
    for (int i = 0; i < 5000; i++) {
        starVerts.push_back(((float)rand() / RAND_MAX) * 20000.0f - 10000.0f);
        starVerts.push_back(((float)rand() / RAND_MAX) * 10000.0f - 5000.0f);
        starVerts.push_back(((float)rand() / RAND_MAX) * 20000.0f - 10000.0f);
    }
    unsigned int starsVAO, starsVBO;
    glGenVertexArrays(1, &starsVAO);
    glGenBuffers(1, &starsVBO);
    glBindVertexArray(starsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, starsVBO);
    glBufferData(GL_ARRAY_BUFFER, starVerts.size() * sizeof(float), starVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int starsTexture = loadTexture("2k_stars.jpg", false);

    glEnable(GL_PROGRAM_POINT_SIZE);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    double totalSimulatedDays = 0.0;
    srand((unsigned int)time(0));
    float cometSpawnTimer = 0.0f;
    bool atlasSpawned = false;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);

        if (!paused) {
            double daysPassed = deltaTime * timeScale;
            totalSimulatedDays += daysPassed;

            for (auto& p : planets) {
                p.currentOrbitAngle += (float)(2.0 * 3.14159265359 * daysPassed / p.orbitalPeriod);
                p.currentRotationAngle += (float)(360.0 * daysPassed / (p.rotationPeriod / 24.0));
                for (auto& m : p.moons) {
                    m.currentAngle += (float)(2.0 * 3.14159265359 * daysPassed / m.orbitalPeriod);
                }
            }

            for (auto& a : asteroids) {
                a.orbitAngle += a.orbitSpeed * deltaTime * timeScale * 0.01f;
                a.rotationAngle += a.rotationSpeed * deltaTime;
            }
        }

        if (focusedPlanet >= 0 && focusedPlanet < (int)planets.size()) {
            auto& p = planets[focusedPlanet];
            float px = p.distance * cos(p.currentOrbitAngle);
            float pz = p.distance * sin(p.currentOrbitAngle);
            glm::vec3 target(px, 0.0f, pz);
            camera.position = target + glm::vec3(0.0f, p.radius * 3.0f, p.radius * 6.0f);
            camera.front = glm::normalize(target - camera.position);
            camera.updateVectors();
        }

        cometSpawnTimer += deltaTime;
        if (cometSpawnTimer > 8.0f && comets.size() < 8) {
            float angle = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;
            float dist = 6000.0f + ((float)rand() / RAND_MAX) * 6000.0f;
            float startX = dist * cos(angle);
            float startZ = dist * sin(angle);
            float startY = ((float)rand() / RAND_MAX - 0.5f) * 1500.0f;
            glm::vec3 pos(startX, startY, startZ);
            glm::vec3 dir = glm::normalize(glm::vec3(0.0f) - pos);
            Comet c;
            c.position = pos;
            c.velocity = dir * (800.0f + ((float)rand() / RAND_MAX) * 1200.0f);
            c.tailLength = 150.0f + ((float)rand() / RAND_MAX) * 200.0f;
            c.life = 25.0f + ((float)rand() / RAND_MAX) * 20.0f;
            c.radius = 1.5f + ((float)rand() / RAND_MAX) * 2.0f;
            c.is3IAtlas = false;
            comets.push_back(c);
            cometSpawnTimer = 0.0f;
        }

        if (!atlasSpawned && currentFrame > 3.0f) {
            spawn3IAtlas(comets, marsOrbitRadius);
            atlasSpawned = true;
        }

        for (auto it = comets.begin(); it != comets.end(); ) {
            it->position += it->velocity * deltaTime;
            it->life -= deltaTime;
            if (it->life <= 0.0f) {
                it = comets.erase(it);
            } else {
                ++it;
            }
        }

        glClearColor(0.0f, 0.0f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.fov), 1600.0f / 900.0f, 0.1f, 20000.0f);
        glm::mat4 view = camera.getViewMatrix();

        glUseProgram(planetShader);
        glUniformMatrix4fv(glGetUniformLocation(planetShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(planetShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glPointSize(1.0f);
        glBindVertexArray(starsVAO);
        glDrawArrays(GL_POINTS, 0, 5000);

        glUseProgram(planetShader);
        glUniform3f(glGetUniformLocation(planetShader, "lightPos"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(planetShader, "lightColor"), 1.0f, 0.95f, 0.8f);
        glUniform3f(glGetUniformLocation(planetShader, "viewPos"), camera.position.x, camera.position.y, camera.position.z);
        glUniform1f(glGetUniformLocation(planetShader, "ambientStrength"), 0.3f);
        glUniform1f(glGetUniformLocation(planetShader, "specularStrength"), 0.0f);
        glUniform1i(glGetUniformLocation(planetShader, "isEmissive"), 1);
        glUniform1f(glGetUniformLocation(planetShader, "emissiveStrength"), 2.0f);
        glUniform1f(glGetUniformLocation(planetShader, "roughness"), 0.5f);
        glUniform1i(glGetUniformLocation(planetShader, "hasNightTexture"), 0);
        glUniform1f(glGetUniformLocation(planetShader, "isRings"), 0.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sunTexture);
        glUniform1i(glGetUniformLocation(planetShader, "diffuseTexture"), 0);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(7.25f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, (float)glfwGetTime() * 0.05f, glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(planetShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(glGetUniformLocation(planetShader, "objectColor"), 1.0f, 1.0f, 1.0f);
        glBindVertexArray(sunMesh.VAO);
        glDrawElements(GL_TRIANGLES, sunMesh.indexCount, GL_UNSIGNED_INT, 0);

        glUniform1i(glGetUniformLocation(planetShader, "isEmissive"), 0);
        glUniform1f(glGetUniformLocation(planetShader, "emissiveStrength"), 0.0f);
        glUniform1f(glGetUniformLocation(planetShader, "ambientStrength"), 0.5f);
        glUniform1f(glGetUniformLocation(planetShader, "specularStrength"), 0.1f);
        glUniform1f(glGetUniformLocation(planetShader, "roughness"), 0.95f);
        glUniform1i(glGetUniformLocation(planetShader, "hasNightTexture"), 0);
        glUniform1f(glGetUniformLocation(planetShader, "isRings"), 0.0f);
        glUniform3f(glGetUniformLocation(planetShader, "objectColor"), 0.55f, 0.5f, 0.45f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glUniform1i(glGetUniformLocation(planetShader, "diffuseTexture"), 0);

        for (auto& a : asteroids) {
            float ax = a.orbitRadius * cos(a.orbitAngle);
            float az = a.orbitRadius * sin(a.orbitAngle);
            glm::mat4 amodel = glm::mat4(1.0f);
            amodel = glm::translate(amodel, glm::vec3(ax, a.yOffset, az));
            amodel = glm::rotate(amodel, glm::radians(a.rotationAngle), a.rotationAxis);
            amodel = glm::scale(amodel, glm::vec3(a.radius));
            glUniformMatrix4fv(glGetUniformLocation(planetShader, "model"), 1, GL_FALSE, glm::value_ptr(amodel));
            glBindVertexArray(asteroidMesh.VAO);
            glDrawElements(GL_TRIANGLES, asteroidMesh.indexCount, GL_UNSIGNED_INT, 0);
        }

        glUseProgram(planetShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glUniform1i(glGetUniformLocation(planetShader, "diffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(planetShader, "hasNightTexture"), 0);
        glUniform1i(glGetUniformLocation(planetShader, "isEmissive"), 1);
        glUniform1f(glGetUniformLocation(planetShader, "emissiveStrength"), 2.5f);
        glUniform1f(glGetUniformLocation(planetShader, "isRings"), 0.0f);
        glUniformMatrix4fv(glGetUniformLocation(planetShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(planetShader, "view"), 1, GL_FALSE, glm::value_ptr(view));

        for (const auto& comet : comets) {
            if (comet.is3IAtlas) {
                glUniform3f(glGetUniformLocation(planetShader, "objectColor"), 0.75f, 0.85f, 1.0f);
            } else {
                glUniform3f(glGetUniformLocation(planetShader, "objectColor"), 0.9f, 0.95f, 1.0f);
            }
            glm::mat4 headModel = glm::mat4(1.0f);
            headModel = glm::translate(headModel, comet.position);
            headModel = glm::scale(headModel, glm::vec3(comet.radius));
            glUniformMatrix4fv(glGetUniformLocation(planetShader, "model"), 1, GL_FALSE, glm::value_ptr(headModel));
            glBindVertexArray(cometHeadMesh.VAO);
            glDrawElements(GL_TRIANGLES, cometHeadMesh.indexCount, GL_UNSIGNED_INT, 0);
        }

        glUseProgram(orbitShader);
        glUniformMatrix4fv(glGetUniformLocation(orbitShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(orbitShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glm::mat4 identityModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(orbitShader, "model"), 1, GL_FALSE, glm::value_ptr(identityModel));

        for (const auto& comet : comets) {
            glm::vec3 tailDir = glm::normalize(comet.position);
            glm::vec3 tailStart = comet.position + tailDir * comet.radius;
            glm::vec3 tailEnd = comet.position + tailDir * comet.tailLength;
            if (comet.is3IAtlas) {
                glUniform3f(glGetUniformLocation(orbitShader, "color"), 0.5f, 0.7f, 1.0f);
                glUniform1f(glGetUniformLocation(orbitShader, "alpha"), 0.7f);
            } else {
                glUniform3f(glGetUniformLocation(orbitShader, "color"), 0.6f, 0.8f, 1.0f);
                glUniform1f(glGetUniformLocation(orbitShader, "alpha"), 0.5f);
            }
            float tailData[6] = {
                tailStart.x, tailStart.y, tailStart.z,
                tailEnd.x, tailEnd.y, tailEnd.z
            };
            glBindBuffer(GL_ARRAY_BUFFER, cometTailVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tailData), tailData);
            glBindVertexArray(cometTailVAO);
            glLineWidth(comet.is3IAtlas ? 5.0f : 3.0f);
            glDrawArrays(GL_LINES, 0, 2);
        }
        glLineWidth(1.0f);

        glUseProgram(planetShader);
        glUniformMatrix4fv(glGetUniformLocation(planetShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(planetShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3f(glGetUniformLocation(planetShader, "lightPos"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(planetShader, "lightColor"), 1.0f, 0.95f, 0.8f);
        glUniform3f(glGetUniformLocation(planetShader, "viewPos"), camera.position.x, camera.position.y, camera.position.z);
        glUniform1i(glGetUniformLocation(planetShader, "isEmissive"), 0);
        glUniform1f(glGetUniformLocation(planetShader, "emissiveStrength"), 0.0f);
        glUniform1f(glGetUniformLocation(planetShader, "ambientStrength"), 0.1f);

        int moonIdx = 0;
        int ringIdx = 0;

        for (size_t i = 0; i < planets.size(); ++i) {
            auto& p = planets[i];
            float px = p.distance * cos(p.currentOrbitAngle);
            float pz = p.distance * sin(p.currentOrbitAngle);
            float py = p.distance * sin(glm::radians(p.inclination)) * sin(p.currentOrbitAngle);

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(px, py, pz));
            glm::vec3 tiltAxis = glm::vec3(sin(glm::radians(p.axialTilt)), cos(glm::radians(p.axialTilt)), 0.0f);
            model = glm::rotate(model, glm::radians(p.currentRotationAngle), tiltAxis);

            glUniformMatrix4fv(glGetUniformLocation(planetShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(glGetUniformLocation(planetShader, "objectColor"), 1.0f, 1.0f, 1.0f);
            glUniform1f(glGetUniformLocation(planetShader, "specularStrength"), p.specular);
            glUniform1f(glGetUniformLocation(planetShader, "roughness"), p.roughness);
            glUniform1i(glGetUniformLocation(planetShader, "hasNightTexture"), p.textures.night != 0 ? 1 : 0);
            glUniform1f(glGetUniformLocation(planetShader, "isRings"), 0.0f);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, p.textures.diffuse);
            glUniform1i(glGetUniformLocation(planetShader, "diffuseTexture"), 0);

            if (p.textures.night != 0) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, p.textures.night);
                glUniform1i(glGetUniformLocation(planetShader, "nightTexture"), 1);
            }

            glBindVertexArray(planetMeshes[i].VAO);
            glDrawElements(GL_TRIANGLES, planetMeshes[i].indexCount, GL_UNSIGNED_INT, 0);

            if (p.hasRings) {
                glUniform1f(glGetUniformLocation(planetShader, "isRings"), 1.0f);
                glEnable(GL_BLEND);
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(px, py, pz));
                model = glm::rotate(model, glm::radians(p.axialTilt), glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                glUniformMatrix4fv(glGetUniformLocation(planetShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, loadTexture("2k_saturn_ring_alpha.png"));
                if (ringIdx < (int)ringMeshes.size()) {
                    glBindVertexArray(ringMeshes[ringIdx].VAO);
                    glDrawElements(GL_TRIANGLES, ringMeshes[ringIdx].indexCount, GL_UNSIGNED_INT, 0);
                }
                glDisable(GL_BLEND);
                ringIdx++;
            }

            for (size_t j = 0; j < p.moons.size(); ++j) {
                auto& m = p.moons[j];
                float mx = px + m.distance * cos(m.currentAngle);
                float mz = pz + m.distance * sin(m.currentAngle);
                float my = py + m.distance * sin(glm::radians(m.inclination)) * sin(m.currentAngle);

                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(mx, my, mz));
                model = glm::rotate(model, (float)glfwGetTime() * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));

                glUniformMatrix4fv(glGetUniformLocation(planetShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glUniform3f(glGetUniformLocation(planetShader, "objectColor"), 1.0f, 1.0f, 1.0f);
                glUniform1f(glGetUniformLocation(planetShader, "specularStrength"), 0.2f);
                glUniform1f(glGetUniformLocation(planetShader, "roughness"), m.roughness);
                glUniform1i(glGetUniformLocation(planetShader, "hasNightTexture"), 0);
                glUniform1f(glGetUniformLocation(planetShader, "isRings"), 0.0f);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m.textures.diffuse);
                glBindVertexArray(moonMeshes[moonIdx].VAO);
                glDrawElements(GL_TRIANGLES, moonMeshes[moonIdx].indexCount, GL_UNSIGNED_INT, 0);
                moonIdx++;
            }
        }

        glUseProgram(orbitShader);
        glUniformMatrix4fv(glGetUniformLocation(orbitShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(orbitShader, "view"), 1, GL_FALSE, glm::value_ptr(view));

        for (auto& p : planets) {
            model = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(orbitShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(glGetUniformLocation(orbitShader, "color"), 0.3f, 0.3f, 0.5f);
            glUniform1f(glGetUniformLocation(orbitShader, "alpha"), 0.3f);
            glBindVertexArray(p.orbitVAO);
            glDrawArrays(GL_LINE_LOOP, 0, 129);

            float px = p.distance * cos(p.currentOrbitAngle);
            float pz = p.distance * sin(p.currentOrbitAngle);
            float py = p.distance * sin(glm::radians(p.inclination)) * sin(p.currentOrbitAngle);

            /*for (auto& m : p.moons) {
                glm::mat4 orbitModel = glm::translate(glm::mat4(1.0f), glm::vec3(px, py, pz));
                glUniformMatrix4fv(glGetUniformLocation(orbitShader, "model"), 1, GL_FALSE, glm::value_ptr(orbitModel));
                glUniform3f(glGetUniformLocation(orbitShader, "color"), 0.2f, 0.2f, 0.4f);
                glUniform1f(glGetUniformLocation(orbitShader, "alpha"), 0.2f);
                glBindVertexArray(m.orbitVAO);
                glDrawArrays(GL_LINE_LOOP, 0, 129);
            }*/
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &sunMesh.VAO);
    glDeleteBuffers(1, &sunMesh.VBO);
    glDeleteBuffers(1, &sunMesh.EBO);

    glfwTerminate();
    return 0;
}
