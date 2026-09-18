#include "boid_rules.h"

namespace {

[[nodiscard]] trivial::math::Vec2f clampLength(trivial::math::Vec2f vector, float maxLength) noexcept {
	const float kLength = vector.length();

	if (kLength <= maxLength) {
		return vector;
	}

	return vector * (maxLength / kLength);
}

[[nodiscard]] trivial::math::Vec2f wrapPosition(trivial::math::Vec2f position,
                                                trivial::math::Vec2f halfExtents) noexcept {
	if (position.x > halfExtents.x) {
		position.x -= 2.0F * halfExtents.x; // NOLINT(readability-magic-numbers)
	} else if (position.x < -halfExtents.x) {
		position.x += 2.0F * halfExtents.x; // NOLINT(readability-magic-numbers)
	}

	if (position.y > halfExtents.y) {
		position.y -= 2.0F * halfExtents.y; // NOLINT(readability-magic-numbers)
	} else if (position.y < -halfExtents.y) {
		position.y += 2.0F * halfExtents.y; // NOLINT(readability-magic-numbers)
	}

	return position;
}

} // namespace

namespace boids {

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,
//             cppcoreguidelines-pro-bounds-constant-array-index)

void computeSeparation(std::span<const Boid> population,
                       std::span<const Boid> batch,
                       const BoidConfig& config,
                       std::span<trivial::math::Vec2f> outSteering) noexcept {
	const float kRadiusSquared = config.separationRadius * config.separationRadius;

	for (std::size_t i = 0; i < batch.size(); ++i) {
		const Boid& boid = batch[i];
		trivial::math::Vec2f accumulated{};

		for (const Boid& kOther : population) {
			if (&kOther == &boid) {
				continue;
			}

			const trivial::math::Vec2f kOffset = boid.position - kOther.position;
			const float kDistanceSquared = kOffset.lengthSquared();

			if (kDistanceSquared > 0.0F && kDistanceSquared < kRadiusSquared) {
				accumulated += kOffset / kDistanceSquared;
			}
		}

		if (accumulated == trivial::math::Vec2f{}) {
			outSteering[i] = {};
			continue;
		}

		const trivial::math::Vec2f kDesired = accumulated.normalised() * config.maxSpeed;

		outSteering[i] = kDesired - boid.velocity;
	}
}

void computeAlignment(std::span<const Boid> population,
                      std::span<const Boid> batch,
                      const BoidConfig& config,
                      std::span<trivial::math::Vec2f> outSteering) noexcept {
	const float kRadiusSquared = config.perceptionRadius * config.perceptionRadius;

	for (std::size_t i = 0; i < batch.size(); ++i) {
		const Boid& boid = batch[i];
		trivial::math::Vec2f velocitySum{};
		float neighbourCount = 0.0F;

		for (const Boid& kOther : population) {
			if (&kOther == &boid) {
				continue;
			}

			const float kDistanceSquared = (boid.position - kOther.position).lengthSquared();

			if (kDistanceSquared < kRadiusSquared) {
				velocitySum += kOther.velocity;
				++neighbourCount;
			}
		}

		if (neighbourCount == 0.0F) {
			outSteering[i] = {};
			continue;
		}

		const trivial::math::Vec2f kAverageVelocity = velocitySum / neighbourCount;
		const trivial::math::Vec2f kDesired = kAverageVelocity.normalisedOrZero() * config.maxSpeed;

		outSteering[i] = kDesired - boid.velocity;
	}
}

void computeCohesion(std::span<const Boid> population,
                     std::span<const Boid> batch,
                     const BoidConfig& config,
                     std::span<trivial::math::Vec2f> outSteering) noexcept {
	const float kRadiusSquared = config.perceptionRadius * config.perceptionRadius;

	for (std::size_t i = 0; i < batch.size(); ++i) {
		const Boid& boid = batch[i];
		trivial::math::Vec2f positionSum{};
		float neighbourCount = 0.0F;

		for (const Boid& kOther : population) {
			if (&kOther == &boid) {
				continue;
			}

			const float kDistanceSquared = (boid.position - kOther.position).lengthSquared();

			if (kDistanceSquared < kRadiusSquared) {
				positionSum += kOther.position;
				++neighbourCount;
			}
		}

		if (neighbourCount == 0.0F) {
			outSteering[i] = {};
			continue;
		}

		const trivial::math::Vec2f kCentroid = positionSum / neighbourCount;
		const trivial::math::Vec2f kDesired = (kCentroid - boid.position).normalisedOrZero() * config.maxSpeed;

		outSteering[i] = kDesired - boid.velocity;
	}
}

void step(std::span<const Boid> batch,
          std::span<const trivial::math::Vec2f> separation,
          std::span<const trivial::math::Vec2f> alignment,
          std::span<const trivial::math::Vec2f> cohesion,
          const BoidConfig& config,
          float deltaTime,
          std::span<Boid> outNext) noexcept {
	for (std::size_t i = 0; i < batch.size(); ++i) {
		const trivial::math::Vec2f kSteeringDelta
		    = clampLength((separation[i] * config.separationWeight) + (alignment[i] * config.alignmentWeight)
		                      + (cohesion[i] * config.cohesionWeight),
		                  config.maxAcceleration * deltaTime);

		const trivial::math::Vec2f kVelocity = clampLength(batch[i].velocity + kSteeringDelta, config.maxSpeed);
		const trivial::math::Vec2f kPosition
		    = wrapPosition(batch[i].position + (kVelocity * deltaTime), config.worldHalfExtents);

		outNext[i] = Boid{.position = kPosition, .velocity = kVelocity};
	}
}

// NOLINTEND(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,
//           cppcoreguidelines-pro-bounds-constant-array-index)

} // namespace boids
