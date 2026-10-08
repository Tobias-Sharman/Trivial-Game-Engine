#ifndef TRIVIAL_ECS_COMPONENT_STORE_H
#define TRIVIAL_ECS_COMPONENT_STORE_H

#include <utility>
#include <vector>

#include <trivial/core/compiler.h>
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

	constexpr void add(Entity entity, const T& component) noexcept {
		const Entity::ValType kIndex = entity.index();

		ensureCapacity(kIndex);

		m_components[kIndex] = component;
		m_active[kIndex] = true;
	}

	constexpr void add(Entity entity, const T&& component) noexcept {
		const Entity::ValType kIndex = entity.index();

		ensureCapacity(kIndex);

		m_components[kIndex] = std::move(component);
		m_active[kIndex] = true;
	}

	constexpr void remove(Entity entity) noexcept {
		const Entity::ValType kIndex = entity.index();

		// NOTE: Would make a point of removing this in release but by the time release matters then the store style
		// will negate this issue
		if (kIndex >= m_active.size()) {
			return;
		}

		m_active[kIndex] = false;
	}

	[[nodiscard]] constexpr bool has(Entity entity) const noexcept {
		const Entity::ValType kIndex = entity.index();

		if (kIndex > m_components.size()) {
			return false;
		}

		return m_active[kIndex];
	}

	// NOTE: No safety check since safety will be enforced when making better storage style
	[[nodiscard]] constexpr T& get(Entity entity) noexcept TRIVIAL_LIFETIMEBOUND {
		return m_components[entity.index()];
	}
	[[nodiscard]] constexpr const T& get(Entity entity) const noexcept TRIVIAL_LIFETIMEBOUND {
		return m_components[entity.index()];
	}

	// TODO: wrap debug helper in macro, not doing now because need to decide if keeping
	[[nodiscard]] constexpr Entity::ValType capacity() const noexcept {
		return static_cast<Entity::ValType>(m_components.size());
	}

private:
	// NOTE: This will be dropped in better implementation
	constexpr void ensureCapacity(Entity::ValType index) noexcept {
		if (index < m_components.size()) {
			return;
		}

		const Entity::ValType kRequiredIndex = index + 1;

		m_components.resize(kRequiredIndex);
		m_active.resize(kRequiredIndex, false);
	}

	std::vector<T> m_components;
	std::vector<bool> m_active;
};

class TRIVIAL_NOVTABLE IComponentStore {
public:
	IComponentStore() = default;

	virtual ~IComponentStore() noexcept;

	IComponentStore(const IComponentStore&) = delete;
	IComponentStore& operator=(const IComponentStore&) = delete;

	IComponentStore(IComponentStore&&) = delete;
	IComponentStore& operator=(IComponentStore&&) = delete;

	virtual void remove(Entity entity) noexcept = 0;
};

template <typename T>
class ErasedComponentStore final : public IComponentStore {
public:
	ErasedComponentStore() = default;
	~ErasedComponentStore() override = default;

	ErasedComponentStore(const ErasedComponentStore&) = delete;
	ErasedComponentStore& operator=(const ErasedComponentStore&) = delete;

	ErasedComponentStore(ErasedComponentStore&&) = delete;
	ErasedComponentStore& operator=(ErasedComponentStore&&) = delete;

	constexpr void remove(Entity entity) noexcept override { store.remove(entity); }

	ComponentStore<T> store;
};

} // namespace trivial::ecs

#endif // TRIVIAL_ECS_COMPONENT_STORE_H
