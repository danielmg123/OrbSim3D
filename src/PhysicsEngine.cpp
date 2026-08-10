#include "PhysicsEngine.hpp"
#include <Eigen/Geometry>

namespace OrbSim3D {

// Body 0 is treated as the star. It obeys the same gravity as everything else,
// but we skip recordTrail() for it so a bright trail does not sit on top of the
// star and clutter the view.

Eigen::Vector3f PhysicsEngine::computeGravity(const Body& a, const Body& b) const
{
    Eigen::Vector3f r   = b.position - a.position;
    float           d2  = r.squaredNorm() + softening;
    Eigen::Vector3f dir = r.normalized();
    float           F   = G * (a.mass * b.mass) / d2;
    return F * dir;
}

void PhysicsEngine::stepEuler(float dt)
{
    const size_t N = bodies.size();
    if (N == 0) return;

    // Sum the pairwise forces into a net acceleration for each body.
    std::vector<Eigen::Vector3f> acc(N, Eigen::Vector3f::Zero());
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = i + 1; j < N; ++j) {
            Eigen::Vector3f F = computeGravity(bodies[i], bodies[j]);
            acc[i] += F / bodies[i].mass;
            acc[j] -= F / bodies[j].mass;
        }
    }

    // Integrate velocity, then position.
    for (size_t i = 0; i < N; ++i) {
        bodies[i].velocity += acc[i] * dt;
        bodies[i].position += bodies[i].velocity * dt;
        if (i > 0) bodies[i].recordTrail();
    }
}

void PhysicsEngine::stepRK4(float dt)
{
    const size_t N = bodies.size();
    if (N < 2) { stepEuler(dt); return; }

    // Snapshot of the current state.
    std::vector<Eigen::Vector3f> x0(N), v0(N);
    for (size_t i = 0; i < N; ++i) {
        x0[i] = bodies[i].position;
        v0[i] = bodies[i].velocity;
    }

    // Accelerations for an arbitrary set of positions.
    auto accel = [&](const std::vector<Eigen::Vector3f>& xs) {
        std::vector<Eigen::Vector3f> a(N, Eigen::Vector3f::Zero());
        for (size_t i = 0; i < N; ++i) {
            for (size_t j = i + 1; j < N; ++j) {
                Body bi = bodies[i]; bi.position = xs[i];
                Body bj = bodies[j]; bj.position = xs[j];
                Eigen::Vector3f F = computeGravity(bi, bj);
                a[i] += F / bodies[i].mass;
                a[j] -= F / bodies[j].mass;
            }
        }
        return a;
    };

    // k1: slope at the start of the interval.
    const auto a1 = accel(x0);
    std::vector<Eigen::Vector3f> k1x(N), k1v(N);
    for (size_t i = 0; i < N; ++i) { k1x[i] = v0[i]; k1v[i] = a1[i]; }

    // k2: slope at the midpoint using k1.
    std::vector<Eigen::Vector3f> x2(N), v2(N);
    for (size_t i = 0; i < N; ++i) {
        x2[i] = x0[i] + 0.5f * dt * k1x[i];
        v2[i] = v0[i] + 0.5f * dt * k1v[i];
    }
    const auto a2 = accel(x2);
    std::vector<Eigen::Vector3f> k2x(N), k2v(N);
    for (size_t i = 0; i < N; ++i) { k2x[i] = v2[i]; k2v[i] = a2[i]; }

    // k3: slope at the midpoint using k2.
    std::vector<Eigen::Vector3f> x3(N), v3(N);
    for (size_t i = 0; i < N; ++i) {
        x3[i] = x0[i] + 0.5f * dt * k2x[i];
        v3[i] = v0[i] + 0.5f * dt * k2v[i];
    }
    const auto a3 = accel(x3);
    std::vector<Eigen::Vector3f> k3x(N), k3v(N);
    for (size_t i = 0; i < N; ++i) { k3x[i] = v3[i]; k3v[i] = a3[i]; }

    // k4: slope at the end of the interval using k3.
    std::vector<Eigen::Vector3f> x4(N), v4(N);
    for (size_t i = 0; i < N; ++i) {
        x4[i] = x0[i] + dt * k3x[i];
        v4[i] = v0[i] + dt * k3v[i];
    }
    const auto a4 = accel(x4);
    std::vector<Eigen::Vector3f> k4x(N), k4v(N);
    for (size_t i = 0; i < N; ++i) { k4x[i] = v4[i]; k4v[i] = a4[i]; }

    // Weighted average of the four slopes.
    for (size_t i = 0; i < N; ++i) {
        bodies[i].position = x0[i] + (dt / 6.0f) *
            (k1x[i] + 2.0f * k2x[i] + 2.0f * k3x[i] + k4x[i]);
        bodies[i].velocity = v0[i] + (dt / 6.0f) *
            (k1v[i] + 2.0f * k2v[i] + 2.0f * k3v[i] + k4v[i]);
        if (i > 0) bodies[i].recordTrail();
    }
}

} // namespace OrbSim3D
