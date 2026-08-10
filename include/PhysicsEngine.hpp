#pragma once

#include <Eigen/Geometry>
#include "Body.hpp"

namespace OrbSim3D {

// N-body gravity solver. Holds the list of bodies and advances them in time
// with either a forward-Euler or a fourth-order Runge-Kutta step.
class PhysicsEngine {
public:
    float G = 6.67430e-11f;   // gravitational constant
    float softening = 1e-3f;  // avoids a singular force at very small separations

    std::vector<Body> bodies;

    void addBody(const Body& b) { bodies.push_back(b); }

    // Advance the whole system by dt.
    void stepEuler(float dt);
    void stepRK4(float dt);

private:
    // Gravitational force on body a due to body b.
    Eigen::Vector3f computeGravity(const Body& a, const Body& b) const;
};

} // namespace OrbSim3D
