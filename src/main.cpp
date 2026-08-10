#include "PhysicsEngine.hpp"
#include "Renderer.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdio>

// Body-size slider value. Renderer.cpp reads this through an extern.
float sizeScale = 1.0f;

// Mouse/camera state for the GLFW callbacks.
static OrbSim3D::Camera* s_camera     = nullptr;
static bool              s_firstMouse = true;
static double            s_lastX      = 0.0;
static double            s_lastY      = 0.0;

// The camera should only orbit while a mouse button is held and the cursor is
// not over an ImGui window.
static bool isOrbitDrag(GLFWwindow* win)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return false;
    bool lmb = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT)  == GLFW_PRESS;
    bool rmb = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    return lmb || rmb;
}

static void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (!isOrbitDrag(window)) { s_firstMouse = true; return; }

    if (s_firstMouse) { s_firstMouse = false; s_lastX = xpos; s_lastY = ypos; }

    float dx = float(xpos - s_lastX);
    float dy = float(s_lastY - ypos);
    s_lastX = xpos;
    s_lastY = ypos;

    if (s_camera) s_camera->processMouseMovement(dx, dy);
}

static void scrollCallback(GLFWwindow*, double, double yoff)
{
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (s_camera) s_camera->processMouseScroll(float(yoff));
}

// Keep the viewport and camera aspect ratio in sync with the window size.
static void framebufferSizeCallback(GLFWwindow* window, int w, int h)
{
    glViewport(0, 0, w, h);
    if (h == 0) return;
    auto cam = static_cast<OrbSim3D::Camera*>(glfwGetWindowUserPointer(window));
    if (cam) cam->setAspect(static_cast<float>(w) / h);
}

int main()
{
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    constexpr int width = 800, height = 600;
    GLFWwindow* window = glfwCreateWindow(width, height, "OrbSim3D", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        return -1;

    std::cout << "OpenGL " << glGetString(GL_VERSION) << '\n';
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.10f, 1.0f);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::StyleColorsDark();

    // Physics setup: a heavy central star plus one planet on a circular orbit.
    OrbSim3D::PhysicsEngine engine;
    engine.G         = 1.0f;
    engine.softening = 0.1f;

    constexpr float M_sun = 100.0f;
    engine.addBody({ M_sun, {0, 0, 0}, {0, 0, 0} });

    constexpr float r0 = 10.0f;
    float v_circ0 = std::sqrt(engine.G * M_sun / r0);  // circular-orbit speed
    engine.addBody({ 1.0f, { r0, 0, 0 }, { 0, v_circ0, 0 } });

    // Snapshot used by the reset controls.
    auto initialBodies = engine.bodies;

    OrbSim3D::Renderer renderer(width, height);
    s_camera = &renderer.getCamera();

    glfwSetWindowUserPointer(window, s_camera);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // Set the viewport for HiDPI / Retina framebuffers on startup.
    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    bool  paused   = false, pHeld = false, rHeld = false;
    float dt       = 0.01f;
    int   maxTrail = 1000;

    // Defaults for the "add body" form.
    float newMass   = 1.0f;
    float newPos[3] = { 5.0f, 0.0f, 0.0f };
    float newVel[3] = { 0.0f, 1.0f, 0.0f };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Hotkeys. Edge-detect the toggles so holding a key doesn't fire every frame.
        bool P = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        if (P && !pHeld) paused = !paused;
        pHeld = P;

        bool R = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (R && !rHeld) {
            engine.bodies = initialBodies;
            for (auto& b : engine.bodies) b.trail.clear();
            paused = false;
        }
        rHeld = R;

        if (glfwGetKey(window, GLFW_KEY_UP)   == GLFW_PRESS) dt *= 1.001f;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) dt *= 0.999f;

        glfwSetInputMode(window, GLFW_CURSOR,
                         isOrbitDrag(window) ? GLFW_CURSOR_DISABLED
                                             : GLFW_CURSOR_NORMAL);

        if (!paused) engine.stepRK4(dt);

        // Follow the center of mass with the camera.
        glm::vec3 com(0.0f);
        float totalMass = 0.0f;
        for (const auto& b : engine.bodies) {
            com += glm::vec3(b.position.x(), b.position.y(), b.position.z()) * b.mass;
            totalMass += b.mass;
        }
        if (totalMass > 0.0f) com /= totalMass;
        renderer.getCamera().setTarget(com);

        char title[128];
        std::snprintf(title, sizeof(title),
                      "OrbSim3D [%s]  dt=%.4f  G=%.3f",
                      paused ? "Paused" : "Running", dt, engine.G);
        glfwSetWindowTitle(window, title);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Controls reference (top-left).
        ImGui::SetNextWindowBgAlpha(0.30f);
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::Begin("Controls", nullptr,
                     ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings);
        ImGui::Text("Drag (LMB/RMB): orbit camera");
        ImGui::Text("Scroll        : zoom");
        ImGui::Separator();
        ImGui::Text("P : Pause / Run");
        ImGui::Text("R : Reset bodies");
        ImGui::Text("Up/Down : tweak dt");
        ImGui::End();

        // Simulation controls (bottom-right).
        const float PAD = 10.0f;
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x - PAD, io.DisplaySize.y - PAD),
            ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::Begin("Simulation");

        ImGui::SliderFloat("G",  &engine.G, 0.0f, 10.0f, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("dt", &dt, 0.001f, 0.05f, "%.4f");
        ImGui::SliderInt("Trail length", &maxTrail, 10, 5000);
        for (auto& b : engine.bodies) b.maxTrail = size_t(maxTrail);

        ImGui::SliderFloat("Size Scale", &sizeScale, 0.1f, 5.0f);

        ImGui::Separator();
        ImGui::Text("Add a new body:");
        ImGui::InputFloat("Mass", &newMass, 0, 0, "%.2f");
        ImGui::InputFloat3("Position", newPos);
        ImGui::InputFloat3("Velocity", newVel);

        if (ImGui::Button("Add Body")) {
            engine.addBody({ newMass,
                             { newPos[0], newPos[1], newPos[2] },
                             { newVel[0], newVel[1], newVel[2] } });
            initialBodies.push_back(engine.bodies.back());
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove Last") && !engine.bodies.empty()) {
            engine.bodies.pop_back();
            if (!initialBodies.empty()) initialBodies.pop_back();
        }

        ImGui::Separator();
        if (ImGui::Button("Reset All Bodies")) {
            engine.bodies = initialBodies;
            for (auto& b : engine.bodies) b.trail.clear();
            paused = false;
        }
        ImGui::End();

        // Render the scene, then the UI on top.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.draw(engine.bodies);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
