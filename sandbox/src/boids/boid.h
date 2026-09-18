#ifndef SANDBOX_BOIDS_BOID_H
#define SANDBOX_BOIDS_BOID_H

#include <cstddef>

#include <trivial/core/math/vec2.h>

namespace boids {

constexpr std::size_t g_kBatchSize = 10;
constexpr std::size_t g_kInitialBoidCount = 10;

struct Boid {
	trivial::math::Vec2f position{};
	trivial::math::Vec2f velocity{};
};

struct BoidConfig {
	float separationRadius = 0.075F; // NOLINT(readability-magic-numbers)
	float perceptionRadius = 0.15F;  // NOLINT(readability-magic-numbers)

	float maxSpeed = 1.0F;        // NOLINT(readability-magic-numbers)
	float maxAcceleration = 0.5F; // NOLINT(readability-magic-numbers)

	float separationWeight = 1.5F; // NOLINT(readability-magic-numbers)
	float alignmentWeight = 1.0F;
	float cohesionWeight = 1.0F;

	trivial::math::Vec2f worldHalfExtents{.x = 1.0F, .y = 1.0F};

	float frameBudgetSeconds = 1.0F / 30.0F; // NOLINT(readability-magic-numbers)
	std::size_t slowFrameStreakToCull = 10;  // NOLINT(readability-magic-numbers)

	float spawnIntervalSeconds = 2.0F; // NOLINT(readability-magic-numbers)
	std::size_t maxBoidCount = 100;    // NOLINT(readability-magic-numbers)
};

} // namespace boids

#endif // SANDBOX_BOIDS_BOID_H
