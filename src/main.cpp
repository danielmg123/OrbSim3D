#include "PhysicsEngine.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Create window
    GLFWwindow* window = glfwCreateWindow(800, 600, "OrbSim3D", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Load OpenGL with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n";

    // Set up the physics engine
    OrbSim3D::PhysicsEngine engine;

    // Create a massive central body (like the Sun)
    engine.addBody({1e6f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}});

    // Create a lighter orbiting body (like a planet)
    engine.addBody({1.0f, {10.0f, 0.0f, 0.0f}, {0.0f, 0.5f, 0.0f}});

    const float dt = 0.01f;  // Simulation timestep

    // Main simulation loop
    while (!glfwWindowShouldClose(window)) {
        // Step the physics simulation
        engine.stepEuler(dt);

        // Print position of the second body for debug
        const auto& pos = engine.bodies[1].position;
        std::cout << "Body 1 Position: (" 
                  << pos.x() << ", " << pos.y() << ", " << pos.z() << ")\n";

        // Clear screen and render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TODO: Hook up my renderer here to draw the bodies and their trails
        // renderer.draw(engine.bodies);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}