#pragma once

#include <Eigen/Geometry>
#include "Body.hpp"

namespace OrbSim3D{

    class PhysicsEngine{
        public:
            // gravitational constant
            float G = 6.67430e-11f;
            float softening = 1e-3f;

            // bodies under simulation
            std::vector<Body> bodies;

            void addBody(const Body& b){
                bodies.push_back(b);
            }

            // perform one Euler time step
            void stepEuler(float dt);

        private:
            // compute force on i due to j
            Eigen::Vector3f computeGravity(const Body& i, const Body& j) const;
    };
}
