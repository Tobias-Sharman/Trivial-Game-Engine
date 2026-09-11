#ifndef TRIVIAL_SRC_CORE_HEAP_ARRAY_H
#define TRIVIAL_SRC_CORE_HEAP_ARRAY_H

#include <cstddef>
#include <memory>

#include <trivial/core/assert.h>

namespace trivial::core {

template <typename T>
class HeapArray {
public:
	explicit HeapArray(const std::size_t kSize) noexcept
	    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
	    : m_elements(std::make_unique<T[]>(kSize))
	    , m_size(kSize) {}

	~HeapArray() noexcept = default;

	HeapArray(const HeapArray&) = delete;
	HeapArray& operator=(const HeapArray&) = delete;

	HeapArray(HeapArray&&) = delete;
	HeapArray& operator=(HeapArray&&) = delete;

	[[nodiscard]] T& operator[](const std::size_t kIndex) noexcept {
		TRIVIAL_ASSERT(kIndex < m_size);
		return m_elements[kIndex]; // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	}

	[[nodiscard]] const T& operator[](const std::size_t kIndex) const noexcept {
		TRIVIAL_ASSERT(kIndex < m_size);
		return m_elements[kIndex]; // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	}

	[[nodiscard]] std::size_t size() const noexcept { return m_size; }

private:
	// TODO: Custom allocator
	std::unique_ptr<T[]> m_elements; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
	std::size_t m_size;
};

} // namespace trivial::core

#endif // TRIVIAL_SRC_CORE_HEAP_ARRAY_H
