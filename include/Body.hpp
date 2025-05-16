#pragma once

#include <Eigen/Core>
#include <cstddef>
#include <vector>

namespace OrbSim3D{
    struct Body{
        float mass;
        Eigen::Vector3f position;
        Eigen::Vector3f velocity;

        // trail of past positions for rendering
        std::vector<Eigen::Vector3f> trail;
        size_t maxTrail = 1000;

        Body(float m, 
            const Eigen::Vector3f& pos = {0, 0, 0}, 
            const Eigen::Vector3f& vel = {0, 0, 0})
            : mass(m), position(pos), velocity(vel)
        {}

        // call this once per frame to record
        void recordTrail(){
            trail.push_back(position);
            if(trail.size() > maxTrail){
                trail.erase(trail.begin());
            }
        }
    };
}