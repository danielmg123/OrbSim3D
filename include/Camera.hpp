#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace OrbSim3D {

// Orbit camera. Position is stored in spherical coordinates (yaw, pitch,
// distance) around a target point, so dragging the mouse simply rotates the
// view around whatever the camera is looking at. The implementation is small
// enough to keep entirely in the header.
class Camera {
public:
    Camera(int width, int height)
        : yaw(90.0f), pitch(-70.0f), distance(30.0f),
          target(0.0f), up(0.0f, 1.0f, 0.0f),
          fov(glm::radians(45.0f)),
          aspect(static_cast<float>(width) / height),
          nearPlane(0.1f), farPlane(100.0f)
    {
        updatePosition();
    }

    [[nodiscard]] glm::mat4 view() const {
        return glm::lookAt(position, target, up);
    }
    [[nodiscard]] glm::mat4 proj() const {
        return glm::perspective(fov, aspect, nearPlane, farPlane);
    }

    // Rotate the camera around its target.
    void processMouseMovement(float dx, float dy) {
        constexpr float sensitivity = 0.1f;
        yaw   += dx * sensitivity;
        pitch += dy * sensitivity;
        pitch = glm::clamp(pitch, -89.0f, 89.0f);  // stop short of the poles
        updatePosition();
    }

    // Zoom in or out with the scroll wheel.
    void processMouseScroll(float yoffset) {
        distance -= yoffset;
        distance = glm::clamp(distance, 1.0f, 100.0f);
        updatePosition();
    }

    // Aim the camera at a new point, for example the system's center of mass.
    void setTarget(const glm::vec3& t) {
        target = t;
        updatePosition();
    }

    void setAspect(float a) { aspect = a; }

private:
    // Recompute the Cartesian position from the spherical coordinates.
    void updatePosition() {
        const float ry = glm::radians(yaw);
        const float rp = glm::radians(pitch);
        position.x = target.x + distance * std::cos(rp) * std::cos(ry);
        position.y = target.y + distance * std::sin(rp);
        position.z = target.z + distance * std::cos(rp) * std::sin(ry);
    }

    glm::vec3 position{};
    glm::vec3 target;
    glm::vec3 up;

    float yaw;        // degrees
    float pitch;      // degrees
    float distance;   // radial distance from the target

    float fov;        // vertical field of view, radians
    float aspect;     // width / height
    float nearPlane;
    float farPlane;
};

} // namespace OrbSim3D
