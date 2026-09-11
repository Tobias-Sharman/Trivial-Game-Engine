#ifndef TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_H
#define TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_H

#include <atomic>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <utility>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/hash/hash.h>
#include <trivial/core/sync/sync_config.h>
#include <trivial/core/thread/thread.h>

#include "core/heap_array.h"
#include "core/sync/bucket.h"
#include "core/sync/parking_lot_slot.h"

// NOTE: For later implementation on systems with a known fixed thread count
//       the global could be dropped for a static version. A lazy initialisation
//       singleton is not pursued, since the gain from one less pointer
//       indirection is insignificant compared to the rest of the function and
//       will make the behaviour less clean and consistent. The global will
//       likely be in lower cache level so fetching is quick anyway

namespace trivial::sync {

class ParkingLot {
public:
	enum class ParkResult : std::uint8_t {
		Unparked,
		Invalidated,
		TimedOut,
	};

	struct UnparkOneResult {
		bool woke;
		bool hasMoreWaiters;
	};

	explicit ParkingLot(const std::size_t kCapacity) noexcept
	    : m_slots(kCapacity)
	    , m_buckets(bucketCountFor(kCapacity))
	    , m_bucketBits(std::countr_zero(bucketCountFor(kCapacity))) {}

	~ParkingLot() noexcept = default;

	ParkingLot(const ParkingLot&) = delete;
	ParkingLot& operator=(const ParkingLot&) = delete;

	ParkingLot(ParkingLot&&) = delete;
	ParkingLot& operator=(ParkingLot&&) = delete;

	template <typename Validate>
	[[nodiscard]] TRIVIAL_FORCE_INLINE ParkResult park(const std::uintptr_t kAddress, Validate&& validate) noexcept {
		const std::size_t kSlotIndex = currentSlotIndex();
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		ParkingLotSlot& slot = m_slots[kSlotIndex];
		TRIVIAL_ASSERT(slot.key.load(std::memory_order_relaxed) == 0);

		Bucket& bucket = bucketFor(kAddress);
		bucket.lock.lock();

		if (!std::forward<Validate>(validate)()) [[unlikely]] {
			bucket.lock.unlock();
			return ParkResult::Invalidated;
		}

		slot.parker.prepare();
		slot.key.store(kAddress, std::memory_order_relaxed);
		pushToQueue(bucket, kSlotIndex);
		bucket.lock.unlock();

		slot.parker.park();
		slot.key.store(0, std::memory_order_relaxed);
		return ParkResult::Unparked;
	}

	template <typename Validate, typename BeforeSleep>
	[[nodiscard]] TRIVIAL_FORCE_INLINE ParkResult park(const std::uintptr_t kAddress,
	                                                   Validate&& validate,
	                                                   BeforeSleep&& beforeSleep) noexcept {
		const std::size_t kSlotIndex = currentSlotIndex();
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		ParkingLotSlot& slot = m_slots[kSlotIndex];
		TRIVIAL_ASSERT(slot.key.load(std::memory_order_relaxed) == 0);

		Bucket& bucket = bucketFor(kAddress);
		bucket.lock.lock();

		if (!std::forward<Validate>(validate)()) [[unlikely]] {
			bucket.lock.unlock();
			return ParkResult::Invalidated;
		}

		slot.parker.prepare();
		slot.key.store(kAddress, std::memory_order_relaxed);
		pushToQueue(bucket, kSlotIndex);
		bucket.lock.unlock();

		std::forward<BeforeSleep>(beforeSleep)();
		slot.parker.park();
		slot.key.store(0, std::memory_order_relaxed);
		return ParkResult::Unparked;
	}

	template <typename Validate>
	[[nodiscard]] TRIVIAL_FORCE_INLINE ParkResult parkFor(const std::uintptr_t kAddress,
	                                                      const std::chrono::nanoseconds kTimeout,
	                                                      Validate&& validate) noexcept {
		const std::size_t kSlotIndex = currentSlotIndex();
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		ParkingLotSlot& slot = m_slots[kSlotIndex];
		TRIVIAL_ASSERT(slot.key.load(std::memory_order_relaxed) == 0);

		Bucket& bucket = bucketFor(kAddress);
		bucket.lock.lock();

		if (!std::forward<Validate>(validate)()) [[unlikely]] {
			bucket.lock.unlock();
			return ParkResult::Invalidated;
		}

		slot.parker.prepare();
		slot.key.store(kAddress, std::memory_order_relaxed);
		pushToQueue(bucket, kSlotIndex);
		bucket.lock.unlock();

		if (slot.parker.parkFor(kTimeout)) {
			slot.key.store(0, std::memory_order_relaxed);
			return ParkResult::Unparked;
		}

		for (;;) {
			const std::uintptr_t kCurrentKey = slot.key.load(std::memory_order_relaxed);
			if (kCurrentKey == 0) {
				return ParkResult::Unparked;
			}

			Bucket& currentBucket = bucketFor(kCurrentKey);
			currentBucket.lock.lock();

			if (slot.key.load(std::memory_order_relaxed) != kCurrentKey) [[unlikely]] {
				currentBucket.lock.unlock();
				continue;
			}

#if TRIVIAL_ENABLE_ASSERTS
			const bool kWasRemoved = removeFromQueue(currentBucket, kSlotIndex);
			TRIVIAL_ASSERT(kWasRemoved);
#else
			removeFromQueue(currentBucket, kSlotIndex);
#endif

			currentBucket.lock.unlock();
			slot.key.store(0, std::memory_order_relaxed);
			return ParkResult::TimedOut;
		}
	}

	template <typename Validate, typename BeforeSleep>
	[[nodiscard]] TRIVIAL_FORCE_INLINE ParkResult parkFor(const std::uintptr_t kAddress,
	                                                      const std::chrono::nanoseconds kTimeout,
	                                                      Validate&& validate,
	                                                      BeforeSleep&& beforeSleep) noexcept {
		const std::size_t kSlotIndex = currentSlotIndex();
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		ParkingLotSlot& slot = m_slots[kSlotIndex];
		TRIVIAL_ASSERT(slot.key.load(std::memory_order_relaxed) == 0);

		Bucket& bucket = bucketFor(kAddress);
		bucket.lock.lock();

		if (!std::forward<Validate>(validate)()) [[unlikely]] {
			bucket.lock.unlock();
			return ParkResult::Invalidated;
		}

		slot.parker.prepare();
		slot.key.store(kAddress, std::memory_order_relaxed);
		pushToQueue(bucket, kSlotIndex);
		bucket.lock.unlock();

		std::forward<BeforeSleep>(beforeSleep)();

		if (slot.parker.parkFor(kTimeout)) {
			slot.key.store(0, std::memory_order_relaxed);
			return ParkResult::Unparked;
		}

		for (;;) {
			const std::uintptr_t kCurrentKey = slot.key.load(std::memory_order_relaxed);
			if (kCurrentKey == 0) {
				return ParkResult::Unparked;
			}

			Bucket& currentBucket = bucketFor(kCurrentKey);
			currentBucket.lock.lock();

			if (slot.key.load(std::memory_order_relaxed) != kCurrentKey) [[unlikely]] {
				currentBucket.lock.unlock();
				continue;
			}

#if TRIVIAL_ENABLE_ASSERTS
			const bool kWasRemoved = removeFromQueue(currentBucket, kSlotIndex);
			TRIVIAL_ASSERT(kWasRemoved);
#else
			removeFromQueue(currentBucket, kSlotIndex);
#endif

			currentBucket.lock.unlock();
			slot.key.store(0, std::memory_order_relaxed);
			return ParkResult::TimedOut;
		}
	}

	[[nodiscard]] bool unparkOne(const std::uintptr_t kAddress) noexcept {
		Bucket& bucket = bucketFor(kAddress);
		bucket.lock.lock();

		std::size_t* link = &bucket.queueHead;
		std::size_t previousIndex = g_kInvalidParkingLotSlotIndex;
		std::size_t currentIndex = bucket.queueHead;

		while (currentIndex != g_kInvalidParkingLotSlotIndex) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			ParkingLotSlot& current = m_slots[currentIndex];

			if (current.key.load(std::memory_order_relaxed) != kAddress) {
				link = &current.nextInQueue;
				previousIndex = currentIndex;
				currentIndex = *link;
				continue;
			}

			*link = current.nextInQueue;

			if (currentIndex == bucket.queueTail) {
				bucket.queueTail = previousIndex;
			}

			current.nextInQueue = g_kInvalidParkingLotSlotIndex;
			current.key.store(0, std::memory_order_relaxed);
			current.parker.beginUnpark().wake();
			bucket.lock.unlock();
			return true;
		}

		bucket.lock.unlock();
		return false;
	}

	template <typename Callback>
	void unparkOne(const std::uintptr_t kAddress, Callback&& callback) noexcept {
		Bucket& bucket = bucketFor(kAddress);
		bucket.lock.lock();

		std::size_t* link = &bucket.queueHead;
		std::size_t previousIndex = g_kInvalidParkingLotSlotIndex;
		std::size_t currentIndex = bucket.queueHead;

		while (currentIndex != g_kInvalidParkingLotSlotIndex) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			ParkingLotSlot& current = m_slots[currentIndex];

			if (current.key.load(std::memory_order_relaxed) != kAddress) {
				link = &current.nextInQueue;
				previousIndex = currentIndex;
				currentIndex = *link;
				continue;
			}

			const std::size_t kNextIndex = current.nextInQueue;
			*link = kNextIndex;

			bool hasMoreWaiters = false;
			if (currentIndex == bucket.queueTail) {
				bucket.queueTail = previousIndex;
			} else {
				std::size_t scanIndex = kNextIndex;
				while (scanIndex != g_kInvalidParkingLotSlotIndex) {
					// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
					ParkingLotSlot& scan = m_slots[scanIndex];
					if (scan.key.load(std::memory_order_relaxed) == kAddress) {
						hasMoreWaiters = true;
						break;
					}
					scanIndex = scan.nextInQueue;
				}
			}

			current.nextInQueue = g_kInvalidParkingLotSlotIndex;
			current.key.store(0, std::memory_order_relaxed);

			std::forward<Callback>(callback)(UnparkOneResult{.woke = true, .hasMoreWaiters = hasMoreWaiters});

			current.parker.beginUnpark().wake();
			bucket.lock.unlock();
			return;
		}

		std::forward<Callback>(callback)(UnparkOneResult{.woke = false, .hasMoreWaiters = false});
		bucket.lock.unlock();
	}

	void unparkAll(const std::uintptr_t kAddress) noexcept {
		Bucket& bucket = bucketFor(kAddress);
		bucket.lock.lock();

		std::size_t* link = &bucket.queueHead;
		std::size_t previousIndex = g_kInvalidParkingLotSlotIndex;
		std::size_t currentIndex = bucket.queueHead;

		while (currentIndex != g_kInvalidParkingLotSlotIndex) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			ParkingLotSlot& current = m_slots[currentIndex];

			if (current.key.load(std::memory_order_relaxed) != kAddress) {
				link = &current.nextInQueue;
				previousIndex = currentIndex;
				currentIndex = *link;
				continue;
			}

			const std::size_t kNextIndex = current.nextInQueue;
			*link = kNextIndex;

			if (currentIndex == bucket.queueTail) {
				bucket.queueTail = previousIndex;
			}

			current.nextInQueue = g_kInvalidParkingLotSlotIndex;
			current.key.store(0, std::memory_order_relaxed);
			current.parker.beginUnpark().wake();

			currentIndex = kNextIndex;
		}

		bucket.lock.unlock();
	}

	template <typename ShouldRequeue>
	[[nodiscard]] bool unparkOneRequeue(const std::uintptr_t kFromAddress,
	                                    const std::uintptr_t kToAddress,
	                                    ShouldRequeue&& shouldRequeue) noexcept {
		Bucket& fromBucket = bucketFor(kFromAddress);
		Bucket& toBucket = bucketFor(kToAddress);
		const bool kSameBucket = (&fromBucket == &toBucket);

		fromBucket.lock.lock();
		if (!kSameBucket) {
			toBucket.lock.lock();
		}

		const bool kRequeue = std::forward<ShouldRequeue>(shouldRequeue)();

		std::size_t* link = &fromBucket.queueHead;
		std::size_t previousIndex = g_kInvalidParkingLotSlotIndex;
		std::size_t currentIndex = fromBucket.queueHead;

		while (currentIndex != g_kInvalidParkingLotSlotIndex) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			ParkingLotSlot& current = m_slots[currentIndex];

			if (current.key.load(std::memory_order_relaxed) != kFromAddress) {
				link = &current.nextInQueue;
				previousIndex = currentIndex;
				currentIndex = *link;
				continue;
			}

			*link = current.nextInQueue;

			if (currentIndex == fromBucket.queueTail) {
				fromBucket.queueTail = previousIndex;
			}

			if (kRequeue) {
				current.key.store(kToAddress, std::memory_order_relaxed);
				pushToQueue(toBucket, currentIndex);
			} else {
				current.nextInQueue = g_kInvalidParkingLotSlotIndex;
				current.key.store(0, std::memory_order_relaxed);
				current.parker.beginUnpark().wake();
			}

			if (!kSameBucket) {
				toBucket.lock.unlock();
			}
			fromBucket.lock.unlock();
			return true;
		}

		if (!kSameBucket) {
			toBucket.lock.unlock();
		}

		fromBucket.lock.unlock();
		return false;
	}

	template <typename ShouldRequeue>
	[[nodiscard]] bool unparkAllRequeue(const std::uintptr_t kFromAddress,
	                                    const std::uintptr_t kToAddress,
	                                    ShouldRequeue&& shouldRequeue) noexcept {
		Bucket& fromBucket = bucketFor(kFromAddress);
		Bucket& toBucket = bucketFor(kToAddress);
		const bool kSameBucket = (&fromBucket == &toBucket);

		fromBucket.lock.lock();
		if (!kSameBucket) {
			toBucket.lock.lock();
		}

		const bool kRequeueAll = std::forward<ShouldRequeue>(shouldRequeue)();

		bool handledAny = false;
		std::size_t* link = &fromBucket.queueHead;
		std::size_t previousIndex = g_kInvalidParkingLotSlotIndex;
		std::size_t currentIndex = fromBucket.queueHead;

		while (currentIndex != g_kInvalidParkingLotSlotIndex) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			ParkingLotSlot& current = m_slots[currentIndex];

			if (current.key.load(std::memory_order_relaxed) != kFromAddress) {
				link = &current.nextInQueue;
				previousIndex = currentIndex;
				currentIndex = *link;
				continue;
			}

			const std::size_t kNextIndex = current.nextInQueue;
			const bool kWakeThisOne = !handledAny && !kRequeueAll;
			handledAny = true;

			if (kWakeThisOne || !kSameBucket) {
				*link = kNextIndex;

				if (currentIndex == fromBucket.queueTail) {
					fromBucket.queueTail = previousIndex;
				}
			} else {
				link = &current.nextInQueue;
				previousIndex = currentIndex;
			}

			if (kWakeThisOne) {
				current.nextInQueue = g_kInvalidParkingLotSlotIndex;
				current.key.store(0, std::memory_order_relaxed);
				current.parker.beginUnpark().wake();
			} else {
				current.key.store(kToAddress, std::memory_order_relaxed);
				if (!kSameBucket) {
					pushToQueue(toBucket, currentIndex);
				}
			}

			currentIndex = kNextIndex;
		}

		if (!kSameBucket) {
			toBucket.lock.unlock();
		}
		fromBucket.lock.unlock();

		return handledAny;
	}

private:
	[[nodiscard]] TRIVIAL_FORCE_INLINE static std::size_t currentSlotIndex() noexcept {
		return trivial::thread::Thread::current()->index();
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE Bucket& bucketFor(const std::uintptr_t kAddress) noexcept {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		return m_buckets[hash::fibonacciHash(kAddress, m_bucketBits)];
	}

	[[nodiscard]] static std::size_t bucketCountFor(const std::size_t kCapacity) noexcept {
		return std::bit_ceil(kCapacity * TRIVIAL_SYNC_PARKING_LOT_LOAD_FACTOR);
	}

	void pushToQueue(Bucket& bucket, const std::size_t kSlotIndex) noexcept {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		m_slots[kSlotIndex].nextInQueue = g_kInvalidParkingLotSlotIndex;

		if (bucket.queueTail == g_kInvalidParkingLotSlotIndex) {
			bucket.queueHead = kSlotIndex;
		} else {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			m_slots[bucket.queueTail].nextInQueue = kSlotIndex;
		}

		bucket.queueTail = kSlotIndex;
	}

	bool removeFromQueue(Bucket& bucket, const std::size_t kSlotIndex) noexcept {
		std::size_t* link = &bucket.queueHead;
		std::size_t previousIndex = g_kInvalidParkingLotSlotIndex;
		std::size_t currentIndex = bucket.queueHead;

		while (currentIndex != g_kInvalidParkingLotSlotIndex) {
			if (currentIndex != kSlotIndex) {
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
				link = &m_slots[currentIndex].nextInQueue;
				previousIndex = currentIndex;
				currentIndex = *link;
				continue;
			}

			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			const std::size_t kNextIndex = m_slots[currentIndex].nextInQueue;
			*link = kNextIndex;

			if (currentIndex == bucket.queueTail) {
				bucket.queueTail = previousIndex;
			}

			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			m_slots[currentIndex].nextInQueue = g_kInvalidParkingLotSlotIndex;
			return true;
		}

		return false;
	}

	trivial::core::HeapArray<ParkingLotSlot> m_slots;
	trivial::core::HeapArray<Bucket> m_buckets;
	int m_bucketBits;
};

namespace detail {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
inline ParkingLot* g_activeParkingLot = nullptr;

} // namespace detail

inline void setActiveParkingLot(ParkingLot* const kParkingLot) noexcept {
	detail::g_activeParkingLot = kParkingLot;
}

[[nodiscard]] inline ParkingLot& activeParkingLot() noexcept {
	TRIVIAL_ASSERT(detail::g_activeParkingLot != nullptr);
	return *detail::g_activeParkingLot;
}

} // namespace trivial::sync

#endif // TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_H
