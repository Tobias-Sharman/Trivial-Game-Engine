#include <trivial/core/memory/allocation.h>

#include <cstddef>
#include <cstring>

#include <gtest/gtest.h>

#include <trivial/core/memory/alignment.h>
#include <trivial/core/memory/byte_count.h>
#include <trivial/core/memory/memory_ops.h>

namespace {

TEST(AllocationTest, AllocateGivesAtLeastTheRequestedSize) {
	constexpr trivial::memory::ByteCount kSize = trivial::memory::bytes(100);

	void* const kPointer = trivial::memory::allocate(kSize);
	ASSERT_NE(kPointer, nullptr);
	EXPECT_GE(trivial::memory::usableSize(kPointer), kSize);
	trivial::memory::fill(kPointer, 0xAB, kSize);

	trivial::memory::free(kPointer);
}

TEST(AllocationTest, AllocateAtLeastWritesBackAUsableSize) {
	constexpr trivial::memory::ByteCount kRequested = trivial::memory::bytes(100);
	trivial::memory::ByteCount size = kRequested;

	void* const kPointer = trivial::memory::allocateAtLeast(size);
	EXPECT_GE(size, kRequested);
	EXPECT_LE(size, trivial::memory::usableSize(kPointer));
	trivial::memory::fill(kPointer, 0xAB, size);

	trivial::memory::free(kPointer);
}

TEST(AllocationTest, AllocateZeroedIsZeroed) {
	constexpr trivial::memory::ByteCount kSize = trivial::memory::bytes(4096);

	void* const kPointer = trivial::memory::allocateZeroed(kSize);
	EXPECT_TRUE(trivial::memory::isZero(kPointer, kSize));

	trivial::memory::free(kPointer);
}

TEST(AllocationTest, AllocateAlignedIsAligned) {
	constexpr trivial::memory::ByteCount kAlignment = trivial::memory::bytes(4096);

	void* const kPointer = trivial::memory::allocateAligned(trivial::memory::bytes(64), kAlignment);
	EXPECT_TRUE(trivial::memory::isAligned(kPointer, kAlignment));

	trivial::memory::free(kPointer);
}

TEST(AllocationTest, TryExpandNeverGrowsPastTheBlock) {
	void* const kPointer = trivial::memory::allocate(trivial::memory::bytes(40));

	EXPECT_FALSE(trivial::memory::tryExpand(kPointer, trivial::memory::bytes(std::size_t{1} << 20U)));

	trivial::memory::free(kPointer);
}

TEST(AllocationTest, FreeOfNullDoesNothing) {
	trivial::memory::free(nullptr);
}

} // namespace
