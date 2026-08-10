#include "Renderer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Global body-size multiplier driven by the ImGui slider in main.cpp.
extern float sizeScale;

namespace OrbSim3D {

static std::string loadFile(const char* path)
{
    std::ifstream in(path, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

GLuint Renderer::compileShader(const char* path, GLenum type)
{
    std::string src  = loadFile(path);
    const char* cSrc = src.c_str();

    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &cSrc, nullptr);
    glCompileShader(s);

    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]{};
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr << "Shader compilation failed for " << path << ":\n"
                  << log << '\n';
    }
    return s;
}

void Renderer::initShaders()
{
    GLuint v = compileShader("shaders/sphere.vert", GL_VERTEX_SHADER);
    GLuint f = compileShader("shaders/sphere.frag", GL_FRAGMENT_SHADER);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, v);
    glAttachShader(shaderProgram, f);
    glLinkProgram(shaderProgram);

    glDeleteShader(v);
    glDeleteShader(f);
}

// crop out the centered square of a non-square image
static void cropToSquare(unsigned char* src, int w, int h, int comp,
                         std::vector<unsigned char>& dst)
{
    int sz = std::min(w, h);
    dst.resize(sz * sz * comp);

    int x0 = (w - sz) / 2;
    int y0 = (h - sz) / 2;
    const unsigned char* rowSrc = src + (y0 * w + x0) * comp;

    for (int y = 0; y < sz; ++y)
        std::memcpy(dst.data() + y * sz * comp,
                    rowSrc + y * w * comp,
                    sz * comp);
}

// Load the six faces into a cubemap. Some of the source images aren't square,
// so anything that isn't gets cropped down before upload.
GLuint Renderer::loadCubemap(const std::vector<std::string>& faces)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);

    stbi_set_flip_vertically_on_load(false);

    int w, h, n;
    for (unsigned i = 0; i < faces.size(); ++i) {
        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &n, 3);
        if (!data) {
            std::cerr << "Failed to load cubemap face: " << faces[i] << '\n';
            continue;
        }

        unsigned char* srcPtr = data;
        std::vector<unsigned char> square;
        if (w != h) {
            cropToSquare(data, w, h, 3, square);
            srcPtr = square.data();
            w = h = std::min(w, h);
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                     0, GL_RGB, w, h, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, srcPtr);
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return tex;
}

void Renderer::initSkybox()
{
    GLuint v = compileShader("shaders/skybox.vert", GL_VERTEX_SHADER);
    GLuint f = compileShader("shaders/skybox.frag", GL_FRAGMENT_SHADER);
    skyboxProgram = glCreateProgram();
    glAttachShader(skyboxProgram, v);
    glAttachShader(skyboxProgram, f);
    glLinkProgram(skyboxProgram);
    glDeleteShader(v);
    glDeleteShader(f);

    glUseProgram(skyboxProgram);
    glUniform1i(glGetUniformLocation(skyboxProgram, "skybox"), 0);
    glUseProgram(0);

    // Unit cube, two triangles per face.
    float cubeVerts[] = {
        // back
        -1, 1,-1,  -1,-1,-1,   1,-1,-1,   1,-1,-1,  1,1,-1, -1,1,-1,
        // left
        -1,-1, 1,  -1,-1,-1,  -1, 1,-1,  -1, 1,-1, -1,1, 1, -1,-1, 1,
        // right
         1,-1,-1,   1,-1, 1,   1, 1, 1,   1, 1, 1,  1,1,-1,  1,-1,-1,
        // front
        -1,-1, 1,  -1, 1, 1,   1, 1, 1,   1, 1, 1,  1,-1, 1, -1,-1, 1,
        // top
        -1, 1,-1,   1, 1,-1,   1, 1, 1,   1, 1, 1, -1,1, 1, -1, 1,-1,
        // bottom
        -1,-1,-1,  -1,-1, 1,   1,-1, 1,   1,-1, 1,  1,-1,-1, -1,-1,-1
    };

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    std::vector<std::string> faces = {
        "assets/skybox/right.jpg",
        "assets/skybox/left.jpg",
        "assets/skybox/top.jpg",
        "assets/skybox/bottom.jpg",
        "assets/skybox/front.jpg",
        "assets/skybox/back.jpg"
    };
    cubemapTex = loadCubemap(faces);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

// Build a UV sphere with position and normal attributes, plus an empty VBO
// that trail points are streamed into each frame.
void Renderer::initSphereMesh(unsigned sectors, unsigned stacks)
{
    std::vector<float>        vertices;
    std::vector<unsigned int> indices;

    for (unsigned i = 0; i <= stacks; ++i) {
        float stackTheta = glm::pi<float>() / 2 - i * (glm::pi<float>() / stacks);
        float xy = std::cos(stackTheta);
        float z  = std::sin(stackTheta);

        for (unsigned j = 0; j <= sectors; ++j) {
            float sectorPhi = j * (glm::two_pi<float>() / sectors);
            float x = xy * std::cos(sectorPhi);
            float y = xy * std::sin(sectorPhi);
            vertices.insert(vertices.end(), {x, y, z,  x, y, z});
        }
    }
    for (unsigned i = 0; i < stacks; ++i) {
        unsigned k1 = i * (sectors + 1);
        unsigned k2 = k1 + sectors + 1;
        for (unsigned j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i)               indices.insert(indices.end(), {k1, k2, k1 + 1});
            if (i != stacks - 1) indices.insert(indices.end(), {k1 + 1, k2, k2 + 1});
        }
    }
    indexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenVertexArrays(1, &trailVAO);
    glGenBuffers(1, &trailVBO);
    glBindVertexArray(trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

Renderer::Renderer(int w, int h) : camera(w, h)
{
    glEnable(GL_DEPTH_TEST);
    initShaders();
    initSphereMesh();
    initSkybox();
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    glDeleteVertexArrays(1, &trailVAO);
    glDeleteBuffers(1, &trailVBO);
    glDeleteProgram(shaderProgram);

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteProgram(skyboxProgram);
    glDeleteTextures(1, &cubemapTex);
}

void Renderer::draw(const std::vector<Body>& bodies)
{
    // Skybox first. Drop the translation from the view matrix so it stays
    // centered on the camera, and render it at the far depth plane.
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glUseProgram(skyboxProgram);

    glm::mat4 view = glm::mat4(glm::mat3(camera.view()));
    glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "view"),
                       1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "projection"),
                       1, GL_FALSE, glm::value_ptr(camera.proj()));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);

    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glBindVertexArray(0);

    // Bodies and their trails.
    glUseProgram(shaderProgram);

    GLuint vLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint pLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(vLoc, 1, GL_FALSE, glm::value_ptr(camera.view()));
    glUniformMatrix4fv(pLoc, 1, GL_FALSE, glm::value_ptr(camera.proj()));

    GLuint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    GLuint lightPosLoc   = glGetUniformLocation(shaderProgram, "lightPos");
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(lightPosLoc,   0.0f, 0.0f, 30.0f);

    GLuint mLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint cLoc = glGetUniformLocation(shaderProgram, "objectColor");

    for (const auto& b : bodies) {
        // Scale radius by the cube root of mass so volume tracks mass.
        float radius = std::cbrt(b.mass) * 0.20f * sizeScale;
        radius = std::max(radius, 0.02f);

        glm::mat4 model = glm::translate(glm::mat4(1.f),
                                         glm::vec3(b.position.x(),
                                                   b.position.y(),
                                                   b.position.z()));
        model = glm::scale(model, glm::vec3(radius));
        glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(cLoc, 1.0f, 0.7f, 0.2f);

        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

        // Trail: identity model matrix, positions are already in world space.
        glUniformMatrix4fv(mLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.f)));

        std::vector<glm::vec3> pts;
        pts.reserve(b.trail.size());
        for (const auto& v : b.trail)
            pts.emplace_back(v.x(), v.y(), v.z());

        glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
        glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(glm::vec3),
                     pts.data(), GL_DYNAMIC_DRAW);

        glUniform3f(cLoc, 1.0f, 1.0f, 1.0f);
        glBindVertexArray(trailVAO);
        glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(pts.size()));
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

} // namespace OrbSim3D
