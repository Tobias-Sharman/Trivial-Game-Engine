#include <trivial/core/memory/page_allocator.h>

#include <cstring>

#include <gtest/gtest.h>

using namespace trivial::memory;

namespace {

class PageAllocatorTest : public ::testing::Test {
protected:
	void SetUp() override {
		bool ok = allocator.init(16 * 1024 * 1024); // 16MB
		ASSERT_TRUE(ok);
	}

	void TearDown() override { allocator.shutdown(); }

	PageAllocator allocator;
};

TEST_F(PageAllocatorTest, GetMoreMemoryReturnsUsablePage) {
	std::size_t pageSize = allocator.pageSize();
	void* ptr = allocator.getMoreMemory(pageSize);
	ASSERT_NE(ptr, nullptr);

	std::memset(ptr, 0xAB, pageSize);
	EXPECT_EQ(static_cast<unsigned char*>(ptr)[0], 0xAB);
	EXPECT_EQ(static_cast<unsigned char*>(ptr)[pageSize - 1], 0xAB);
}

TEST_F(PageAllocatorTest, RequestExceedingReservationReturnsNullptr) {
	EXPECT_EQ(allocator.getMoreMemory(1024ull * 1024 * 1024), nullptr);
}

TEST(PageAllocatorStandaloneTest, RequestExactlyFillingReservationSucceeds) {
	PageAllocator small;
	ASSERT_TRUE(small.init(1)); // rounds up to exactly one page internally
	EXPECT_NE(small.getMoreMemory(small.pageSize()), nullptr);
	small.shutdown();
}

TEST_F(PageAllocatorTest, OomHandlerFiresWithCorrectInfo) {
	static bool handlerFired = false;
	static std::size_t capturedSize = 0;
	handlerFired = false;

	allocator.setOomHandler([](const OomInfo& info) {
		handlerFired = true;
		capturedSize = info.requestedSize;
	});

	std::size_t oversized = 1024ull * 1024 * 1024;
	EXPECT_EQ(allocator.getMoreMemory(oversized), nullptr);
	EXPECT_TRUE(handlerFired);
	EXPECT_EQ(capturedSize, oversized);
}

TEST_F(PageAllocatorTest, DecommitOnValidRangeSucceeds) {
	void* ptr = allocator.getMoreMemory(allocator.pageSize());
	ASSERT_NE(ptr, nullptr);
	EXPECT_TRUE(allocator.decommit(ptr, allocator.pageSize()));
}

TEST(PageAllocatorStandaloneTest, OsLevelReservationFailureReturnsFalseAndFiresHandler) {
	static bool handlerFired = false;
	static std::size_t capturedSize = 0;
	handlerFired = false;

	PageAllocator allocator;
	allocator.setOomHandler([](const OomInfo& info) {
		handlerFired = true;
		capturedSize = info.requestedSize;
	});

	// Larger than any real system's addressable virtual space —
	// should fail at the OS level, not just our own bounds check
	// (which doesn't apply yet, since init() hasn't reserved anything).
	std::size_t absurdSize = SIZE_MAX / 2;
	bool ok = allocator.init(absurdSize);

	EXPECT_FALSE(ok);
	EXPECT_TRUE(handlerFired);
	EXPECT_EQ(capturedSize, absurdSize);

	// Confirm the failed init left state clean enough for a real one to follow.
	bool retryOk = allocator.init(4096);
	EXPECT_TRUE(retryOk);
	allocator.shutdown();
}

} // namespace
