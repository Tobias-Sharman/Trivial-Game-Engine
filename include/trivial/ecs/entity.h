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
	static constexpr std::uint32_t s_kIndexBits = 24;
	static constexpr std::uint32_t s_kGenerationBits = 8;
	static constexpr std::uint32_t s_kGenerationShift = s_kIndexBits;

	static_assert(s_kIndexBits > 0);
	static_assert(s_kGenerationBits > 0);
	static_assert(s_kIndexBits + s_kGenerationBits == std::numeric_limits<ValType>::digits);

	static constexpr ValType s_kIndexMask = (ValType{1} << s_kIndexBits) - ValType{1};
	static constexpr ValType s_kGenerationMask = ~s_kIndexMask;

	static constexpr ValType s_kMaxGeneration = (ValType{1} << s_kGenerationBits) - ValType{1};
	static constexpr ValType s_kInvalidIndex = s_kIndexMask;

	constexpr Entity() = default;

	// TODO: Cover delete, copy, move, etc. ro5/6

	[[nodiscard]] static constexpr Entity make(ValType index, ValType generation) {
		return Entity{(generation << s_kGenerationShift) | index};
	}

	[[nodiscard]] static constexpr Entity fromValue(ValType value) { return Entity{value}; }

	[[nodiscard]] constexpr bool valid() const { return index() != s_kInvalidIndex; }

	[[nodiscard]] constexpr ValType index() const { return m_value & s_kIndexMask; }
	[[nodiscard]] constexpr ValType generation() const { return m_value >> s_kGenerationShift; }
	[[nodiscard]] constexpr ValType value() const { return m_value; }

	[[nodiscard]] constexpr bool operator==(const Entity&) const = default;

private:
	explicit constexpr Entity(ValType value)
	    : m_value(value) {}

	ValType m_value = s_kInvalidIndex;
};

} // namespace trivial::ecs

#endif // TRIVIAL_ECS_ENTITY_H
