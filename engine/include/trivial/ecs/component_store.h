#ifndef TRIVIAL_ECS_COMPONENT_STORE_H
#define TRIVIAL_ECS_COMPONENT_STORE_H

#include <utility>
#include <vector>

#include <trivial/core/compiler.h>
#include <trivial/ecs/entity.h>

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

	[[nodiscard]] constexpr T& get(Entity entity) noexcept TRIVIAL_LIFETIMEBOUND {
		return m_components[entity.index()];
	}
	[[nodiscard]] constexpr const T& get(Entity entity) const noexcept TRIVIAL_LIFETIMEBOUND {
		return m_components[entity.index()];
	}

	[[nodiscard]] constexpr Entity::ValType capacity() const noexcept {
		return static_cast<Entity::ValType>(m_components.size());
	}

private:
	constexpr void ensureCapacity(Entity::ValType index) noexcept {
		if (index < m_components.size()) {
			return;
		}

		const Entity::ValType kRequiredSize = index + 1;

		m_components.resize(kRequiredSize);
		m_active.resize(kRequiredSize, false);
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
