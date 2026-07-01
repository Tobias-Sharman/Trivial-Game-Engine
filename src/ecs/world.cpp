#include <trivial/ecs/world.h>

#include "core/assert.h"
#include "core/log.h"
#include "core/profile.h"

namespace trivial::ecs {

Entity World::create() {
	TRIVIAL_PROFILE_FUNCTION();

	if (!m_freeIndices.empty()) {
		const Entity::ValType kIndex = m_freeIndices.back();
		m_freeIndices.pop_back();

		TRIVIAL_ASSERT(kIndex < m_entities.size());
		TRIVIAL_ASSERT(!m_entities[kIndex].alive);
		TRIVIAL_ASSERT(m_entities[kIndex].generation <= Entity::kMaxGeneration);

		m_entities[kIndex].alive = true;
		++m_aliveCount;

		return Entity::make(kIndex, m_entities[kIndex].generation);
	}

	// TODO: Profile this in release when testing game to see if worth dropping check
	if (m_entities.size() >= Entity::kInvalidIndex) {
		TRIVIAL_LOG_ERROR("Maximum entity count reached");

		return Entity{};
	}

	const Entity::ValType kIndex = static_cast<Entity::ValType>(m_entities.size());

	m_entities.push_back(EntitySlot{
	    .generation = 0,
	    .alive = true,
	});

	++m_aliveCount;

	return Entity::make(kIndex, 0);
}

void World::destroy(Entity entity) {
	TRIVIAL_PROFILE_FUNCTION();

	if (!alive(entity)) {
		TRIVIAL_LOG_WARNING("Tried to delete an entity that is not alive, review algorithm");

		return;
	}

	const Entity::ValType kIndex = entity.index();
	EntitySlot& slot = m_entities[kIndex];

	m_positions2D.remove(entity);
	m_velocities2D.remove(entity);

	for (auto& [_, store] : m_componentStores) {
		store->remove(entity);
	}

	slot.alive = false;
	--m_aliveCount;

	if (slot.generation == Entity::kMaxGeneration) {
		TRIVIAL_LOG_WARNING("Hit the max generation, think about increasing the number of bits for generation");

		return;
	}

	++slot.generation;
	m_freeIndices.push_back(kIndex);
}

bool World::alive(Entity entity) const {
	if (!entity.valid()) {
		return false;
	}

	const Entity::ValType kIndex = entity.index();

	if (kIndex >= m_entities.size()) {
		return false;
	}

	const EntitySlot& slot = m_entities[kIndex];

	return slot.alive && slot.generation == entity.generation();
}

} // namespace trivial::ecs
