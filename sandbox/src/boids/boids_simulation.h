#ifndef SANDBOX_BOIDS_BOIDS_SIMULATION_H
#define SANDBOX_BOIDS_BOIDS_SIMULATION_H

#include <cstddef>
#include <random>
#include <vector>

#include <trivial/gpu/context.h>
#include <trivial/render/renderer.h>
#include <trivial/rhi/mesh_types.h>
#include <trivial/task/task_handle.h>

#include "boid.h"

namespace boids {

class BoidsSimulation {
public:
	void init(trivial::gpu::Context* gpu, std::size_t initialBoidCount) noexcept;

	void update(float deltaTime) noexcept;

	[[nodiscard]] const std::vector<trivial::render::Drawable>& drawables() const noexcept { return m_drawables; }

	[[nodiscard]] std::size_t boidCount() const noexcept { return m_boids.size(); }

private:
	void updateCullDecision(float deltaTime) noexcept;
	void updateSpawnDecision(float deltaTime) noexcept;

	void applyPendingCull() noexcept;
	void applyPendingSpawn() noexcept;

	void resizeWorkingBuffers() noexcept;
	void rebuildDrawables() noexcept;

	BoidConfig m_config{};

	std::vector<Boid> m_boids;
	std::vector<Boid> m_boidsNext;

	std::vector<trivial::math::Vec2f> m_separation;
	std::vector<trivial::math::Vec2f> m_alignment;
	std::vector<trivial::math::Vec2f> m_cohesion;

	std::vector<trivial::render::Drawable> m_drawables;
	trivial::rhi::MeshHandle m_mesh = 0;

	std::vector<trivial::task::TaskHandle> m_frameHandles;

	std::mt19937 m_rng{std::random_device{}()};

	std::size_t m_slowFrameStreak = 0;
	bool m_pendingCull = false;

	float m_timeSinceLastSpawn = 0.0F;
	bool m_pendingSpawn = false;
};

} // namespace boids

#endif // SANDBOX_BOIDS_BOIDS_SIMULATION_H
