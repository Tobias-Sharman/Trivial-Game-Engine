#ifndef TRIVIAL_SRC_CORE_HEAP_ARRAY_H
#define TRIVIAL_SRC_CORE_HEAP_ARRAY_H

#include <cstddef>
#include <memory>
#include <span>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>

namespace trivial::core {

template <typename T>
class HeapArray {
public:
	explicit constexpr HeapArray(std::size_t size) noexcept
	    : m_elements(std::make_unique<T[]>(size)) // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
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

	[[nodiscard]] constexpr std::span<T>::iterator begin() noexcept TRIVIAL_LIFETIMEBOUND {
		return std::span<T>{m_elements.get(), m_size}.begin();
	}

	[[nodiscard]] constexpr std::span<const T>::iterator begin() const noexcept TRIVIAL_LIFETIMEBOUND {
		return std::span<const T>{m_elements.get(), m_size}.begin();
	}

	[[nodiscard]] constexpr std::span<T>::iterator end() noexcept TRIVIAL_LIFETIMEBOUND {
		return std::span<T>{m_elements.get(), m_size}.end();
	}

	[[nodiscard]] constexpr std::span<const T>::iterator end() const noexcept TRIVIAL_LIFETIMEBOUND {
		return std::span<const T>{m_elements.get(), m_size}.end();
	}

private:
	std::unique_ptr<T[]> m_elements; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
	std::size_t m_size;
};

} // namespace trivial::core

#endif // TRIVIAL_SRC_CORE_HEAP_ARRAY_H
