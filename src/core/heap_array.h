#ifndef TRIVIAL_SRC_CORE_HEAP_ARRAY_H
#define TRIVIAL_SRC_CORE_HEAP_ARRAY_H

#include <cstddef>
#include <memory>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>

namespace trivial::core {

template <typename T>
class HeapArray {
public:
	explicit constexpr HeapArray(std::size_t size) noexcept
	    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
	    : m_elements(std::make_unique<T[]>(size))
	    , m_size(size) {}

	~HeapArray() noexcept = default;

	HeapArray(const HeapArray&) = delete;
	HeapArray& operator=(const HeapArray&) = delete;

	HeapArray(HeapArray&&) = delete;
	HeapArray& operator=(HeapArray&&) = delete;

	[[nodiscard]] constexpr T& operator[](std::size_t index) noexcept TRIVIAL_LIFETIMEBOUND {
		TRIVIAL_ASSERT(index < m_size);
		return m_elements[index];
	}

	[[nodiscard]] constexpr const T& operator[](std::size_t index) const noexcept TRIVIAL_LIFETIMEBOUND {
		TRIVIAL_ASSERT(index < m_size);
		return m_elements[index];
	}

	[[nodiscard]] constexpr std::size_t size() const noexcept { return m_size; }

private:
	// TODO: Custom allocator
	std::unique_ptr<T[]> m_elements; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
	std::size_t m_size;
};

} // namespace trivial::core

#endif // TRIVIAL_SRC_CORE_HEAP_ARRAY_H
