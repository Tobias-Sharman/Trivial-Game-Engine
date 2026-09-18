#include "boids_simulation.h"

#include <array>
#include <cmath>
#include <numbers>
#include <span>

#include <trivial/core/math/affine2.h>
#include <trivial/core/math/angle.h>
#include <trivial/task/task.h>

#include "boid_rules.h"

namespace {

[[nodiscard]] trivial::rhi::MeshHandle createBoidMesh(trivial::gpu::Context* gpu) noexcept {
	constexpr float kNoseX = 0.02F;
	constexpr float kTailX = -0.015F;
	constexpr float kTailHalfWidth = 0.01F;

	const std::array<trivial::rhi::Vertex2, 3> kVertices = {
	    {
	        {.position = {.x = kNoseX, .y = 0.0F}, .colour = {255, 0, 0, 255}},
	        {.position = {.x = kTailX, .y = kTailHalfWidth}, .colour = {0, 255, 0, 255}},
	        {.position = {.x = kTailX, .y = -kTailHalfWidth}, .colour = {0, 0, 255, 255}},
	    },
	};
	const std::array<std::uint16_t, 3> kIndices = {0, 1, 2};

	return gpu->createMesh(kVertices, kIndices);
}

[[nodiscard]] boids::Boid makeRandomBoid(std::mt19937& rng, trivial::math::Vec2f halfExtents, float speed) noexcept {
	std::uniform_real_distribution<float> positionXDist(-halfExtents.x, halfExtents.x);
	std::uniform_real_distribution<float> positionYDist(-halfExtents.y, halfExtents.y);
	// NOLINTNEXTLINE(readability-magic-numbers)
	std::uniform_real_distribution<float> angleDist(0.0F, 2.0F * std::numbers::pi_v<float>);

	const float kAngle = angleDist(rng);

	return {
	    .position = {.x = positionXDist(rng), .y = positionYDist(rng)},
	    .velocity = {.x = std::cos(kAngle) * speed, .y = std::sin(kAngle) * speed},
	};
}

} // namespace

namespace boids {

void BoidsSimulation::init(trivial::gpu::Context* gpu, std::size_t initialBoidCount) noexcept {
	m_mesh = createBoidMesh(gpu);

	const std::size_t kBatchCount = (initialBoidCount + g_kBatchSize - 1) / g_kBatchSize;

	m_boids.resize(kBatchCount * g_kBatchSize);

	for (Boid& boid : m_boids) {
		boid = makeRandomBoid(m_rng, m_config.worldHalfExtents, m_config.maxSpeed);
	}

	resizeWorkingBuffers();
	rebuildDrawables();
}

void BoidsSimulation::update(float deltaTime) noexcept {
	const std::size_t kBatchCount = m_boids.size() / g_kBatchSize;

	std::vector<trivial::task::TaskHandle> stepHandles;
	stepHandles.reserve(kBatchCount);

	m_frameHandles.clear();
	m_frameHandles.reserve((kBatchCount * 4) + 2);

	const std::span<const Boid> kPopulation = m_boids;
	const BoidConfig& config = m_config;

	const trivial::task::TaskLaunchOptions kRuleOptions{
	    .affinity = trivial::task::TaskAffinity::AnyWorker,
	    .priority = trivial::task::TaskPriority::High,
	};

	for (std::size_t batchIndex = 0; batchIndex < kBatchCount; ++batchIndex) {
		const std::size_t kStart = batchIndex * g_kBatchSize;

		const std::span<const Boid> kBatch = kPopulation.subspan(kStart, g_kBatchSize);
		const std::span<trivial::math::Vec2f> kSeparationOut = std::span{m_separation}.subspan(kStart, g_kBatchSize);
		const std::span<trivial::math::Vec2f> kAlignmentOut = std::span{m_alignment}.subspan(kStart, g_kBatchSize);
		const std::span<trivial::math::Vec2f> kCohesionOut = std::span{m_cohesion}.subspan(kStart, g_kBatchSize);
		const std::span<Boid> kNextBatch = std::span{m_boidsNext}.subspan(kStart, g_kBatchSize);

		const trivial::task::TaskHandle kSeparationHandle = trivial::task::launch(
		    trivial::task::TaskPayload{[kPopulation, kBatch, &config, kSeparationOut]() noexcept {
			    computeSeparation(kPopulation, kBatch, config, kSeparationOut);
		    }},
		    kRuleOptions);

		const trivial::task::TaskHandle kAlignmentHandle = trivial::task::launch(
		    trivial::task::TaskPayload{[kPopulation, kBatch, &config, kAlignmentOut]() noexcept {
			    computeAlignment(kPopulation, kBatch, config, kAlignmentOut);
		    }},
		    kRuleOptions);

		const trivial::task::TaskHandle kCohesionHandle
		    = trivial::task::launch(trivial::task::TaskPayload{[kPopulation, kBatch, &config, kCohesionOut]() noexcept {
			                            computeCohesion(kPopulation, kBatch, config, kCohesionOut);
		                            }},
		                            kRuleOptions);

		const std::array<trivial::task::TaskHandle, 3> kRuleHandles{
		    kSeparationHandle,
		    kAlignmentHandle,
		    kCohesionHandle,
		};

		const trivial::task::TaskHandle kStepHandle = trivial::task::launch(
		    trivial::task::TaskPayload{
		        [kBatch, kSeparationOut, kAlignmentOut, kCohesionOut, &config, deltaTime, kNextBatch]() noexcept {
			        step(kBatch, kSeparationOut, kAlignmentOut, kCohesionOut, config, deltaTime, kNextBatch);
		        }},
		    std::span<const trivial::task::TaskHandle>{kRuleHandles},
		    kRuleOptions);

		stepHandles.push_back(kStepHandle);

		m_frameHandles.push_back(kSeparationHandle);
		m_frameHandles.push_back(kAlignmentHandle);
		m_frameHandles.push_back(kCohesionHandle);
		m_frameHandles.push_back(kStepHandle);
	}

	const trivial::task::TaskLaunchOptions kAuxOptions{
	    .affinity = trivial::task::TaskAffinity::AnyWorker,
	    .priority = trivial::task::TaskPriority::Normal,
	};

	const trivial::task::TaskHandle kCullHandle
	    = trivial::task::launch(trivial::task::TaskPayload{[this, deltaTime]() noexcept {
		                            updateCullDecision(deltaTime);
	                            }},
	                            std::span<const trivial::task::TaskHandle>{stepHandles},
	                            kAuxOptions);

	const trivial::task::TaskHandle kSpawnHandle
	    = trivial::task::launch(trivial::task::TaskPayload{[this, deltaTime]() noexcept {
		                            updateSpawnDecision(deltaTime);
	                            }},
	                            std::span<const trivial::task::TaskHandle>{stepHandles},
	                            kAuxOptions);

	m_frameHandles.push_back(kCullHandle);
	m_frameHandles.push_back(kSpawnHandle);

	const std::array<trivial::task::TaskHandle, 2> kFinalHandles{kCullHandle, kSpawnHandle};

	trivial::task::wait(std::span<const trivial::task::TaskHandle>{kFinalHandles});

	for (trivial::task::TaskHandle handle : m_frameHandles) {
		(void)trivial::task::release(handle);
	}

	std::swap(m_boids, m_boidsNext);

	applyPendingCull();
	applyPendingSpawn();

	resizeWorkingBuffers();
	rebuildDrawables();
}

void BoidsSimulation::updateCullDecision(float deltaTime) noexcept {
	if (deltaTime > m_config.frameBudgetSeconds) {
		++m_slowFrameStreak;
	} else {
		m_slowFrameStreak = 0;
	}

	m_pendingCull = m_slowFrameStreak >= m_config.slowFrameStreakToCull && m_boids.size() > g_kBatchSize;
}

void BoidsSimulation::updateSpawnDecision(float deltaTime) noexcept {
	m_timeSinceLastSpawn += deltaTime;

	m_pendingSpawn = m_timeSinceLastSpawn >= m_config.spawnIntervalSeconds && m_boids.size() < m_config.maxBoidCount;
}

void BoidsSimulation::applyPendingCull() noexcept {
	if (!m_pendingCull) {
		return;
	}

	m_boids.resize(m_boids.size() - g_kBatchSize);

	m_slowFrameStreak = 0;
	m_pendingCull = false;
}

void BoidsSimulation::applyPendingSpawn() noexcept {
	if (!m_pendingSpawn) {
		return;
	}

	for (std::size_t i = 0; i < g_kBatchSize; ++i) {
		m_boids.push_back(makeRandomBoid(m_rng, m_config.worldHalfExtents, m_config.maxSpeed));
	}

	m_timeSinceLastSpawn = 0.0F;
	m_pendingSpawn = false;
}

void BoidsSimulation::resizeWorkingBuffers() noexcept {
	m_boidsNext.resize(m_boids.size());
	m_separation.resize(m_boids.size());
	m_alignment.resize(m_boids.size());
	m_cohesion.resize(m_boids.size());
}

void BoidsSimulation::rebuildDrawables() noexcept {
	m_drawables.clear();
	m_drawables.reserve(m_boids.size());

	for (const Boid& boid : m_boids) {
		const trivial::math::Anglef kHeading
		    = trivial::math::Anglef::fromRadians(std::atan2(boid.velocity.y, boid.velocity.x));

		m_drawables.push_back({
		    .mesh = m_mesh,
		    .transform
		    = trivial::math::Affine2f::translation(boid.position) * trivial::math::Affine2f::rotation(kHeading),
		    .tint = {.x = 1.0F, .y = 1.0F, .z = 1.0F, .w = 1.0F},
		});
	}
}

} // namespace boids
