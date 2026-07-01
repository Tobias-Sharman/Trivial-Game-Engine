#ifndef TRIVIAL_SCENE_SCENE_H
#define TRIVIAL_SCENE_SCENE_H

#include <trivial/ecs/world.h>

namespace trivial {

class Scene {
public:
	Scene() = default;

	~Scene() = default;

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	Scene(Scene&&) = delete;
	Scene& operator=(Scene&&) = delete;

	[[nodiscard]] ecs::World& world() { return m_world; }
	[[nodiscard]] const ecs::World& world() const { return m_world; }

private:
	ecs::World m_world;
};

} // namespace trivial

#endif // TRIVIAL_SCENE_SCENE_H
