#ifndef TRIVIAL_ECS_ENTITY_H
#define TRIVIAL_ECS_ENTITY_H

#include <cstdint>
#include <limits>

#include <trivial/ecs/ecs_config.h>

#define TRIVIAL_ECS_ENTITY_GENERATION_SHIFT TRIVIAL_ECS_ENTITY_INDEX_BITS

#define TRIVIAL_ECS_ENTITY_INDEX_MASK ((std::uint32_t{1} << TRIVIAL_ECS_ENTITY_INDEX_BITS) - std::uint32_t{1})
#define TRIVIAL_ECS_ENTITY_GENERATION_MASK (~TRIVIAL_ECS_ENTITY_INDEX_MASK)

namespace trivial::ecs {

// NOTE: Need to add asserts in use as is unsafe - see make, index, and generation
struct Entity {
	using ValType = std::uint32_t;

	static_assert(TRIVIAL_ECS_ENTITY_INDEX_BITS + TRIVIAL_ECS_ENTITY_GENERATION_BITS
	                  == std::numeric_limits<ValType>::digits,
	              "Entity index and generation bits must exactly fill the entity value");

	constexpr Entity() = default;

	// TODO: Cover delete, copy, move, etc. ro5/6

	[[nodiscard]] static constexpr Entity make(ValType index, ValType generation) noexcept {
		return Entity{(generation << TRIVIAL_ECS_ENTITY_GENERATION_SHIFT) | index};
	}

	[[nodiscard]] static constexpr Entity fromValue(ValType value) noexcept { return Entity{value}; }

	[[nodiscard]] constexpr bool valid() const noexcept { return index() != TRIVIAL_ECS_ENTITY_INVALID_INDEX; }

	[[nodiscard]] constexpr ValType index() const noexcept { return m_value & TRIVIAL_ECS_ENTITY_INDEX_MASK; }
	[[nodiscard]] constexpr ValType generation() const noexcept {
		return m_value >> TRIVIAL_ECS_ENTITY_GENERATION_SHIFT;
	}
	[[nodiscard]] constexpr ValType value() const noexcept { return m_value; }

	[[nodiscard]] constexpr bool operator==(const Entity&) const noexcept = default;

private:
	explicit constexpr Entity(ValType value) noexcept
	    : m_value(value) {}

	ValType m_value = TRIVIAL_ECS_ENTITY_INVALID_INDEX;
};

} // namespace trivial::ecs

#undef TRIVIAL_ECS_ENTITY_GENERATION_SHIFT
#undef TRIVIAL_ECS_ENTITY_INDEX_MASK
#undef TRIVIAL_ECS_ENTITY_GENERATION_MASK

#endif // TRIVIAL_ECS_ENTITY_H
