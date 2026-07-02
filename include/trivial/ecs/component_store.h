#ifndef TRIVIAL_ECS_COMPONENT_STORE_H
#define TRIVIAL_ECS_COMPONENT_STORE_H

#include <utility>
#include <vector>

#include <trivial/ecs/entity.h>

// NOTE: Remeber not all cache lines are 64 bytes, will want some special handling for 128 byte for m series chips
//       Do not care about any server cpu that may have more
//       When adding in support for gpu compute will need to take care for their different cache size too

namespace trivial::ecs {

template <typename T>
class ComponentStore {
public:
	ComponentStore() = default;

	~ComponentStore() = default;

	ComponentStore(const ComponentStore&) = delete;
	ComponentStore& operator=(const ComponentStore&) = delete;

	ComponentStore(ComponentStore&&) = delete;
	ComponentStore& operator=(ComponentStore&&) = delete;

	void add(Entity entity, const T& component) {
		const Entity::ValType kIndex = entity.index();

		ensureCapacity(kIndex);

		m_components[kIndex] = component;
		m_active[kIndex] = true;
	}

	void add(Entity entity, const T&& component) {
		const Entity::ValType kIndex = entity.index();

		ensureCapacity(kIndex);

		m_components[kIndex] = std::move(component);
		m_active[kIndex] = true;
	}

	void remove(Entity entity) {
		const Entity::ValType kIndex = entity.index();

		// NOTE: Would make a point of removing this in release but by the time release matters then the store style
		// will negate this issue
		if (kIndex >= m_active.size()) {
			return;
		}

		m_active[kIndex] = false;
	}

	[[nodiscard]] bool has(Entity entity) const {
		const Entity::ValType kIndex = entity.index();

		if (kIndex > m_components.size()) {
			return false;
		}

		return m_active[kIndex];
	}

	// NOTE: No safety check since safety will be enforced when making better storage style
	[[nodiscard]] T& get(Entity entity) { return m_components[entity.index()]; }
	[[nodiscard]] const T& get(Entity entity) const { return m_components[entity.index()]; }

	// TODO: wrap debug helper in macro, not doing now because need to decide if keeping
	[[nodiscard]] Entity::ValType capacity() const { return static_cast<Entity::ValType>(m_components.size()); }

private:
	// NOTE: This will be dropped in better implementation
	void ensureCapacity(Entity::ValType index) {
		if (index < m_components.size()) {
			return;
		}

		auto requiredIndex = index + 1;

		m_components.resize(requiredIndex);
		m_active.resize(requiredIndex, false);
	}

	std::vector<T> m_components;
	std::vector<bool> m_active;
};

class IComponentStore {
public:
	IComponentStore() = default;

	virtual ~IComponentStore() = default;

	IComponentStore(const IComponentStore&) = delete;
	IComponentStore& operator=(const IComponentStore&) = delete;

	IComponentStore(IComponentStore&&) = delete;
	IComponentStore& operator=(IComponentStore&&) = delete;

	virtual void remove(Entity entity) = 0;
};

template <typename T>
class ErasedComponentStore : public IComponentStore {
public:
	ErasedComponentStore() = default;
	~ErasedComponentStore() override = default;

	ErasedComponentStore(const ErasedComponentStore&) = delete;
	ErasedComponentStore& operator=(const ErasedComponentStore&) = delete;

	ErasedComponentStore(ErasedComponentStore&&) = delete;
	ErasedComponentStore& operator=(ErasedComponentStore&&) = delete;

	void remove(Entity entity) override { store.remove(entity); }

	ComponentStore<T> store;
};

} // namespace trivial::ecs

#endif // TRIVIAL_ECS_COMPONENT_STORE_H
