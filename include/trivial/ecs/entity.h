#ifndef TRIVIAL_ECS_ENTITY_H
#define TRIVIAL_ECS_ENTITY_H

#include <cstdint>
#include <limits>

namespace trivial::ecs {

// NOTE: Need to add asserts in use as is unsafe - see make, index, and generation
class Entity {
public:
	using ValType = std::uint32_t;

	// TODO: Decide what is the best split
	static constexpr std::uint32_t kIndexBits = 24;
	static constexpr std::uint32_t kGenerationBits = 8;
	static constexpr std::uint32_t kGenerationShift = kIndexBits;

	static_assert(kIndexBits > 0);
	static_assert(kGenerationBits > 0);
	static_assert(kIndexBits + kGenerationBits == std::numeric_limits<ValType>::digits);

	static constexpr ValType kIndexMask = (ValType{1} << kIndexBits) - ValType{1};
	static constexpr ValType kGenerationMask = ~kIndexMask;

	static constexpr ValType kMaxGeneration = (ValType{1} << kGenerationBits) - ValType{1};
	static constexpr ValType kInvalidIndex = kIndexMask;

	constexpr Entity() = default;

	// TODO: Cover delete, copy, move, etc. ro5/6

	[[nodiscard]] static constexpr Entity make(ValType index, ValType generation) {
		return Entity{(generation << kGenerationShift) | index};
	}

	[[nodiscard]] static constexpr Entity fromValue(ValType value) { return Entity{value}; }

	[[nodiscard]] constexpr bool valid() const { return index() != kInvalidIndex; }

	[[nodiscard]] constexpr ValType index() const { return m_value & kIndexMask; }
	[[nodiscard]] constexpr ValType generation() const { return m_value >> kGenerationShift; }
	[[nodiscard]] constexpr ValType value() const { return m_value; }

	[[nodiscard]] constexpr bool operator==(const Entity&) const = default;

private:
	explicit constexpr Entity(ValType value)
	    : m_value(value) {}

	ValType m_value = kInvalidIndex;
};

} // namespace trivial::ecs

#endif // TRIVIAL_ECS_ENTITY_H
