#ifndef TRIVIAL_ECS_WORLD_H
#define TRIVIAL_ECS_WORLD_H

#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include <trivial/ecs/component_store.h>
#include <trivial/ecs/components.h>
#include <trivial/ecs/entity.h>

namespace trivial::ecs {

class World {
public:
	World() = default;

	~World() = default;

	World(const World&) = delete;
	World& operator=(const World&) = delete;

	World(World&&) = delete;
	World& operator=(World&&) = delete;

	[[nodiscard]] Entity create();
	void destroy(Entity entity);

	[[nodiscard]] bool alive(Entity entity) const;

	[[nodiscard]] Entity::ValType aliveCount() const { return m_aliveCount; }
	[[nodiscard]] Entity::ValType capacity() const { return static_cast<Entity::ValType>(m_entities.size()); }

	template <typename T>
	void add(Entity entity, const T& component) {
		if (!alive(entity)) {
			return;
		}

		if constexpr (isStandardComponent<T>()) {
			standardStore<T>().add(entity, component);
		} else {
			getOrCreateDynamicStore<T>().add(entity, component);
		}
	}

	template <typename T>
	void add(Entity entity, T&& component) {
		if (!alive(entity)) {
			return;
		}

		if constexpr (isStandardComponent<T>()) {
			standardStore<T>().add(entity, std::forward<T>(component));
		} else {
			getOrCreateDynamicStore<T>().add(entity, std::forward<T>(component));
		}
	}

	template <typename T>
	void remove(Entity entity) {
		if constexpr (isStandardComponent<T>()) {
			standardStore<T>().remove(entity);
		} else {
			ComponentStore<T>* store = findDynamicStore<T>();

			if (store == nullptr) {
				return;
			}

			store->remove(entity);
		}
	}

	template <typename T>
	[[nodiscard]] bool has(Entity entity) const {
		if (!alive(entity)) {
			return false;
		}

		if constexpr (isStandardComponent<T>()) {
			return standardStore<T>().has(entity);
		} else {
			const ComponentStore<T>* store = findDynamicStore<T>();

			return store != nullptr && store->has(entity);
		}
	}

	template <typename T>
	[[nodiscard]] T& get(Entity entity) {
		if constexpr (isStandardComponent<T>()) {
			return standardStore<T>().get(entity);
		} else {
			ComponentStore<T>* store = findDynamicStore<T>();

			return store->get(entity);
		}
	}

	template <typename T>
	[[nodiscard]] const T& get(Entity entity) const {
		if constexpr (isStandardComponent<T>()) {
			return standardStore<T>().get(entity);
		} else {
			const ComponentStore<T>* store = findDynamicStore<T>();

			return store->get(entity);
		}
	}

private:
	struct EntitySlot {
		Entity::ValType generation = 0;
		bool alive = false; // TODO: Drop on redesign of entity storage style, should be able to encode in storage
	};

	template <typename T>
	[[nodiscard]] static constexpr bool isStandardComponent() {
		return std::is_same_v<T, Position2D> || std::is_same_v<T, Velocity2D>;
	}

	// TODO: Can probably make this cleaner and more scaleable for more types, do when adding them

	template <typename T>
	[[nodiscard]] ComponentStore<T>& standardStore() {
		if constexpr (std::is_same_v<T, Position2D>) {
			return m_positions2D;
		} else if constexpr (std::is_same_v<T, Velocity2D>) {
			return m_velocities2D;
		}
	}

	template <typename T>
	[[nodiscard]] const ComponentStore<T>& standardStore() const {
		if constexpr (std::is_same_v<T, Position2D>) {
			return m_positions2D;
		} else if constexpr (std::is_same_v<T, Velocity2D>) {
			return m_velocities2D;
		}
	}

	template <typename T>
	[[nodiscard]] ComponentStore<T>* findDynamicStore() {
		const auto kIt = m_componentStores.find(std::type_index(typeid(T)));

		if (kIt == m_componentStores.end()) {
			return nullptr;
		}

		IComponentStore* erasedStore = kIt->second.get();

		auto* typedStore = static_cast<ErasedComponentStore<T>*>(erasedStore);
		ComponentStore<T>& componentStore = typedStore->store;

		return &componentStore;
	}

	template <typename T>
	[[nodiscard]] const ComponentStore<T>* findDynamicStore() const {
		const auto kIt = m_componentStores.find(std::type_index(typeid(T)));

		if (kIt == m_componentStores.end()) {
			return nullptr;
		}

		const IComponentStore* erasedStore = kIt->second.get();

		const auto* typedStore = static_cast<const ErasedComponentStore<T>*>(erasedStore);
		const ComponentStore<T>& componentStore = typedStore->store;

		return &componentStore;
	}

	template <typename T>
	[[nodiscard]] ComponentStore<T>& getOrCreateDynamicStore() {
		ComponentStore<T>* existingStore = findDynamicStore<T>();

		if (existingStore != nullptr) {
			return *existingStore;
		}

		auto erasedStore = std::make_unique<ErasedComponentStore<T>>();
		ComponentStore<T>& newStore = erasedStore->store;

		m_componentStores.emplace(std::type_index(typeid(T)), std::move(erasedStore));

		return newStore;
	}

	std::vector<EntitySlot> m_entities;
	std::vector<Entity::ValType> m_freeIndices;
	// TODO: Confirm that using a std::stack is defo worse than a vector as a stack

	Entity::ValType m_aliveCount = 0; // TODO: Also encode into storage style

	ComponentStore<Position2D> m_positions2D;
	ComponentStore<Velocity2D> m_velocities2D;

	// TODO: Custom type id would make this faster as could put into a vector
	//       Would need to consider how this affects serialisation so leave until then
	std::unordered_map<std::type_index, std::unique_ptr<IComponentStore>> m_componentStores;
};

} // namespace trivial::ecs

#endif // TRIVIAL_ECS_WORLD_H
