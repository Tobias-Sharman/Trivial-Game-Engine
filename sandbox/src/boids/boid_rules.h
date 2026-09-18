#ifndef SANDBOX_BOIDS_BOID_RULES_H
#define SANDBOX_BOIDS_BOID_RULES_H

#include <span>

#include "boid.h"

namespace boids {

void computeSeparation(std::span<const Boid> population,
                       std::span<const Boid> batch,
                       const BoidConfig& config,
                       std::span<trivial::math::Vec2f> outSteering) noexcept;

void computeAlignment(std::span<const Boid> population,
                      std::span<const Boid> batch,
                      const BoidConfig& config,
                      std::span<trivial::math::Vec2f> outSteering) noexcept;

void computeCohesion(std::span<const Boid> population,
                     std::span<const Boid> batch,
                     const BoidConfig& config,
                     std::span<trivial::math::Vec2f> outSteering) noexcept;

void step(std::span<const Boid> batch,
          std::span<const trivial::math::Vec2f> separation,
          std::span<const trivial::math::Vec2f> alignment,
          std::span<const trivial::math::Vec2f> cohesion,
          const BoidConfig& config,
          float deltaTime,
          std::span<Boid> outNext) noexcept;

} // namespace boids

#endif // SANDBOX_BOIDS_BOID_RULES_H
