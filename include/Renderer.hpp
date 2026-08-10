#pragma once

#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Body.hpp"
#include "Camera.hpp"

namespace OrbSim3D {

// Draws the scene: a cubemap skybox, one lit sphere per body, and a line
// strip for each body's trail. Owns the camera so main can wire up input
// callbacks to it.
class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    void draw(const std::vector<Body>& bodies);

    Camera& getCamera() { return camera; }

private:
    // Spheres and trails.
    GLuint shaderProgram{};
    GLuint sphereVAO{}, sphereVBO{}, sphereEBO{};
    GLuint trailVAO{},  trailVBO{};
    size_t indexCount{0};

    // Skybox.
    GLuint skyboxVAO{}, skyboxVBO{};
    GLuint skyboxProgram{};
    GLuint cubemapTex{};

    Camera camera;

    void   initShaders();
    void   initSphereMesh(unsigned int sectorCount = 36,
                          unsigned int stackCount  = 18);
    GLuint compileShader(const char* path, GLenum type);

    void   initSkybox();
    GLuint loadCubemap(const std::vector<std::string>& faces);
};

} // namespace OrbSim3D
