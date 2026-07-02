#ifndef TRIVIAL_WORLD_WORLD_CONTEXT_H
#define TRIVIAL_WORLD_WORLD_CONTEXT_H

#include <trivial/world/world.h>

namespace trivial {

class WorldContext {
public:
	WorldContext() = default;

	~WorldContext() = default;

	WorldContext(const WorldContext&) = delete;
	WorldContext& operator=(const WorldContext&) = delete;

	WorldContext(WorldContext&&) = delete;
	WorldContext& operator=(WorldContext&&) = delete;

	[[nodiscard]] world::World& world() { return m_world; }
	[[nodiscard]] const world::World& world() const { return m_world; }

private:
	world::World m_world;
};

} // namespace trivial

#endif // TRIVIAL_WORLD_WORLD_CONTEXT_H
