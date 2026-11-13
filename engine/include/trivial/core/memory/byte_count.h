#ifndef TRIVIAL_CORE_MEMORY_BYTE_COUNT_H
#define TRIVIAL_CORE_MEMORY_BYTE_COUNT_H

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <trivial/core/assert.h>
#include <trivial/core/bits/bits.h>
#include <trivial/core/compiler.h>
#include <trivial/core/memory/memory_constants.h>

namespace trivial::memory {

struct ByteCount {
	TRIVIAL_FORCE_INLINE constexpr ByteCount() noexcept = default;

	[[nodiscard]] TRIVIAL_FORCE_INLINE static constexpr ByteCount fromBytes(std::uint64_t bytes) noexcept {
		return ByteCount(bytes);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t count() const noexcept { return m_bytes; }

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::size_t toSize() const noexcept {
		return static_cast<std::size_t>(m_bytes);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t toKiB() const noexcept {
		return m_bytes / TRIVIAL_MEMORY_BYTES_PER_KIB;
	}
	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t toMiB() const noexcept {
		return m_bytes / TRIVIAL_MEMORY_BYTES_PER_MIB;
	}
	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t toGiB() const noexcept {
		return m_bytes / TRIVIAL_MEMORY_BYTES_PER_GIB;
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t toKB() const noexcept {
		return m_bytes / TRIVIAL_MEMORY_BYTES_PER_KB;
	}
	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t toMB() const noexcept {
		return m_bytes / TRIVIAL_MEMORY_BYTES_PER_MB;
	}
	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t toGB() const noexcept {
		return m_bytes / TRIVIAL_MEMORY_BYTES_PER_GB;
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr double toKiBDouble() const noexcept {
		return static_cast<double>(m_bytes) / static_cast<double>(TRIVIAL_MEMORY_BYTES_PER_KIB);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr double toMiBDouble() const noexcept {
		return static_cast<double>(m_bytes) / static_cast<double>(TRIVIAL_MEMORY_BYTES_PER_MIB);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr double toGiBDouble() const noexcept {
		return static_cast<double>(m_bytes) / static_cast<double>(TRIVIAL_MEMORY_BYTES_PER_GIB);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr double toKBDouble() const noexcept {
		return static_cast<double>(m_bytes) / static_cast<double>(TRIVIAL_MEMORY_BYTES_PER_KB);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr double toMBDouble() const noexcept {
		return static_cast<double>(m_bytes) / static_cast<double>(TRIVIAL_MEMORY_BYTES_PER_MB);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr double toGBDouble() const noexcept {
		return static_cast<double>(m_bytes) / static_cast<double>(TRIVIAL_MEMORY_BYTES_PER_GB);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool isZero() const noexcept { return m_bytes == 0; }

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool isPowerOfTwo() const noexcept {
		return bits::isPowerOfTwo(m_bytes);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount operator+(ByteCount rhs) const noexcept {
		return ByteCount(m_bytes + rhs.m_bytes);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount operator-(ByteCount rhs) const noexcept {
		return ByteCount(m_bytes - rhs.m_bytes);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount operator*(std::uint64_t scalar) const noexcept {
		return ByteCount(m_bytes * scalar);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount operator/(std::uint64_t scalar) const noexcept {
		TRIVIAL_ASSUME(scalar != 0);
		return ByteCount(m_bytes / scalar);
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr std::uint64_t operator/(ByteCount rhs) const noexcept {
		TRIVIAL_ASSUME(rhs.m_bytes != 0);
		return m_bytes / rhs.m_bytes;
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount operator%(ByteCount rhs) const noexcept {
		TRIVIAL_ASSUME(rhs.m_bytes != 0);
		return ByteCount(m_bytes % rhs.m_bytes);
	}

	TRIVIAL_FORCE_INLINE constexpr ByteCount& operator+=(ByteCount rhs) noexcept {
		m_bytes += rhs.m_bytes;
		return *this;
	}

	TRIVIAL_FORCE_INLINE constexpr ByteCount& operator-=(ByteCount rhs) noexcept {
		m_bytes -= rhs.m_bytes;
		return *this;
	}

	TRIVIAL_FORCE_INLINE constexpr ByteCount& operator*=(std::uint64_t scalar) noexcept {
		m_bytes *= scalar;
		return *this;
	}

	TRIVIAL_FORCE_INLINE constexpr ByteCount& operator/=(std::uint64_t scalar) noexcept {
		TRIVIAL_ASSUME(scalar != 0);
		m_bytes /= scalar;
		return *this;
	}

	TRIVIAL_FORCE_INLINE constexpr ByteCount& operator%=(ByteCount rhs) noexcept {
		TRIVIAL_ASSUME(rhs.m_bytes != 0);
		m_bytes %= rhs.m_bytes;
		return *this;
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool operator==(const ByteCount&) const noexcept = default;

	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool operator<(ByteCount rhs) const noexcept {
		return m_bytes < rhs.m_bytes;
	}
	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool operator<=(ByteCount rhs) const noexcept {
		return m_bytes <= rhs.m_bytes;
	}
	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool operator>(ByteCount rhs) const noexcept {
		return m_bytes > rhs.m_bytes;
	}
	[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr bool operator>=(ByteCount rhs) const noexcept {
		return m_bytes >= rhs.m_bytes;
	}

private:
	TRIVIAL_FORCE_INLINE constexpr explicit ByteCount(std::uint64_t bytes) noexcept
	    : m_bytes(bytes) {}

	std::uint64_t m_bytes = 0;
};

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount operator*(std::uint64_t scalar, ByteCount bytes) noexcept {
	return bytes * scalar;
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount bytes(std::uint64_t count) noexcept {
	return ByteCount::fromBytes(count);
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount kibibytes(std::uint64_t count) noexcept {
	return ByteCount::fromBytes(count * TRIVIAL_MEMORY_BYTES_PER_KIB);
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount mebibytes(std::uint64_t count) noexcept {
	return ByteCount::fromBytes(count * TRIVIAL_MEMORY_BYTES_PER_MIB);
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount gibibytes(std::uint64_t count) noexcept {
	return ByteCount::fromBytes(count * TRIVIAL_MEMORY_BYTES_PER_GIB);
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount kilobytes(std::uint64_t count) noexcept {
	return ByteCount::fromBytes(count * TRIVIAL_MEMORY_BYTES_PER_KB);
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount megabytes(std::uint64_t count) noexcept {
	return ByteCount::fromBytes(count * TRIVIAL_MEMORY_BYTES_PER_MB);
}

[[nodiscard]] TRIVIAL_FORCE_INLINE constexpr ByteCount gigabytes(std::uint64_t count) noexcept {
	return ByteCount::fromBytes(count * TRIVIAL_MEMORY_BYTES_PER_GB);
}

static_assert(sizeof(std::size_t) == sizeof(std::uint64_t), "ByteCount::toSize assumes a 64-bit size_t");

static_assert(sizeof(ByteCount) == sizeof(std::uint64_t));
static_assert(alignof(ByteCount) == alignof(std::uint64_t));

static_assert(std::is_trivially_copyable_v<ByteCount>);
static_assert(std::is_standard_layout_v<ByteCount>);
static_assert(std::is_nothrow_default_constructible_v<ByteCount>);

static_assert(!std::is_aggregate_v<ByteCount>);
static_assert(!std::is_constructible_v<ByteCount, std::uint64_t>);
static_assert(!std::is_convertible_v<std::uint64_t, ByteCount>);

} // namespace trivial::memory

#endif // TRIVIAL_CORE_MEMORY_BYTE_COUNT_H
