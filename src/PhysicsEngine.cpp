#include "PhysicsEngine.hpp"
#include <Eigen/Geometry> // for normalized

namespace OrbSim3D{
    Eigen::Vector3f PhysicsEngine::computeGravity(const Body& a, const Body& b) const{
        Eigen::Vector3f r = b.position - a.position;
        float dist2 = r.squaredNorm() + softening;
        Eigen::Vector3f dir = r.normalized();
        float magnitude = G * (a.mass * b.mass) / dist2;
        return magnitude * dir;
    }

    void PhysicsEngine::stepEuler(float dt){
        size_t N = bodies.size();
        // store accelerations:
        std::vector<Eigen::Vector3f> acc(N, Eigen::Vector3f::Zero());

        // compute net forces
        for(size_t i = 0; i < N; ++i){
            for(size_t j = i+1; j < N; ++j){
                auto Fij = computeGravity(bodies[i], bodies[j]);
                acc[i] += Fij / bodies[i].mass;
                acc[j] -= Fij / bodies[j].mass;
            }
        }

        // integrate velocities and position
        for(size_t i = 0; i < N; ++i){
            bodies[i].velocity += acc[i] * dt;
            bodies[i].position += bodies[i].velocity * dt;
            bodies[i].recordTrail();
        }
    }
    
}