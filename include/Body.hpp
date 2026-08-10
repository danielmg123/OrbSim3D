#pragma once

#include <Eigen/Core>
#include <cstddef>
#include <vector>

namespace OrbSim3D {

// A point mass in the simulation. Each body keeps a short history of its
// past positions so the renderer can draw an orbit trail behind it.
struct Body {
    float mass;
    Eigen::Vector3f position;
    Eigen::Vector3f velocity;

    std::vector<Eigen::Vector3f> trail;
    size_t maxTrail = 1000;

    Body(float m,
         const Eigen::Vector3f& pos = {0, 0, 0},
         const Eigen::Vector3f& vel = {0, 0, 0})
        : mass(m), position(pos), velocity(vel)
    {}

    // Append the current position to the trail, dropping the oldest point
    // once the trail reaches its maximum length.
    void recordTrail() {
        trail.push_back(position);
        if (trail.size() > maxTrail)
            trail.erase(trail.begin());
    }
};

} // namespace OrbSim3D
