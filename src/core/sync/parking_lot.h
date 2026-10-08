#ifndef TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_H
#define TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_H

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include <trivial/core/assert.h>
#include <trivial/core/compiler.h>
#include <trivial/core/hash/hash.h>
#include <trivial/core/sync/sync_config.h>
#include <trivial/core/thread/thread.h>
#include <trivial/core/time/duration.h>

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

	struct UnparkAllRequeueResult {
		bool handledAny;
		bool anyRequeued;
	};

	explicit ParkingLot(std::size_t capacity) noexcept
	    : m_slots(capacity)
	    , m_buckets(bucketCountFor(capacity))
	    , m_bucketBits(static_cast<std::uint32_t>(std::countr_zero(bucketCountFor(capacity)))) {}

	~ParkingLot() noexcept = default;

	ParkingLot(const ParkingLot&) = delete;
	ParkingLot& operator=(const ParkingLot&) = delete;

	ParkingLot(ParkingLot&&) = delete;
	ParkingLot& operator=(ParkingLot&&) = delete;

	template <typename Validate>
	[[nodiscard]] TRIVIAL_FORCE_INLINE ParkResult park(std::uintptr_t address, Validate&& validate) noexcept {
		const std::size_t kSlotIndex = currentSlotIndex();
		ParkingLotSlot& slot = m_slots[kSlotIndex];
		TRIVIAL_ASSERT(slot.key.load(std::memory_order_relaxed) == 0);

		Bucket& bucket = bucketFor(address);
		bucket.lock.lock();

		if (!std::forward<Validate>(validate)()) [[unlikely]] {
			bucket.lock.unlock();
			return ParkResult::Invalidated;
		}

		slot.parker.prepare();
		slot.key.store(address, std::memory_order_relaxed);
		pushToQueue(bucket, kSlotIndex);
		bucket.lock.unlock();

		slot.parker.park();
		slot.key.store(0, std::memory_order_relaxed);
		return ParkResult::Unparked;
	}

	template <typename Validate, typename BeforeSleep>
	[[nodiscard]] TRIVIAL_FORCE_INLINE ParkResult park(std::uintptr_t address,
	                                                   Validate&& validate,
	                                                   BeforeSleep&& beforeSleep) noexcept {
		const std::size_t kSlotIndex = currentSlotIndex();
		ParkingLotSlot& slot = m_slots[kSlotIndex];
		TRIVIAL_ASSERT(slot.key.load(std::memory_order_relaxed) == 0);

		Bucket& bucket = bucketFor(address);
		bucket.lock.lock();

		if (!std::forward<Validate>(validate)()) [[unlikely]] {
			bucket.lock.unlock();
			return ParkResult::Invalidated;
		}

		slot.parker.prepare();
		slot.key.store(address, std::memory_order_relaxed);
		pushToQueue(bucket, kSlotIndex);
		bucket.lock.unlock();

		std::forward<BeforeSleep>(beforeSleep)();
		slot.parker.park();
		slot.key.store(0, std::memory_order_relaxed);
		return ParkResult::Unparked;
	}

	template <typename Validate>
	[[nodiscard]] TRIVIAL_FORCE_INLINE ParkResult parkFor(std::uintptr_t address,
	                                                      time::Duration timeout,
	                                                      Validate&& validate) noexcept {
		const std::size_t kSlotIndex = currentSlotIndex();
		ParkingLotSlot& slot = m_slots[kSlotIndex];
		TRIVIAL_ASSERT(slot.key.load(std::memory_order_relaxed) == 0);

		Bucket& bucket = bucketFor(address);
		bucket.lock.lock();

		if (!std::forward<Validate>(validate)()) [[unlikely]] {
			bucket.lock.unlock();
			return ParkResult::Invalidated;
		}

		slot.parker.prepare();
		slot.key.store(address, std::memory_order_relaxed);
		pushToQueue(bucket, kSlotIndex);
		bucket.lock.unlock();

		if (slot.parker.parkFor(timeout)) {
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

			TRIVIAL_VERIFY(removeFromQueue(currentBucket, kSlotIndex));

			currentBucket.lock.unlock();
			slot.key.store(0, std::memory_order_relaxed);
			return ParkResult::TimedOut;
		}
	}

	// clang-format off
	template <typename Validate, typename BeforeSleep>
	[[nodiscard]] TRIVIAL_FORCE_INLINE ParkResult parkFor(std::uintptr_t address,
	                                                      time::Duration timeout,
	                                                      Validate&& validate,
	                                                      BeforeSleep&& beforeSleep) noexcept {
		// clang-format on
		const std::size_t kSlotIndex = currentSlotIndex();
		ParkingLotSlot& slot = m_slots[kSlotIndex];
		TRIVIAL_ASSERT(slot.key.load(std::memory_order_relaxed) == 0);

		Bucket& bucket = bucketFor(address);
		bucket.lock.lock();

		if (!std::forward<Validate>(validate)()) [[unlikely]] {
			bucket.lock.unlock();
			return ParkResult::Invalidated;
		}

		slot.parker.prepare();
		slot.key.store(address, std::memory_order_relaxed);
		pushToQueue(bucket, kSlotIndex);
		bucket.lock.unlock();

		std::forward<BeforeSleep>(beforeSleep)();

		if (slot.parker.parkFor(timeout)) {
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

			TRIVIAL_VERIFY(removeFromQueue(currentBucket, kSlotIndex));

			currentBucket.lock.unlock();
			slot.key.store(0, std::memory_order_relaxed);
			return ParkResult::TimedOut;
		}
	}

	[[nodiscard]] bool unparkOne(std::uintptr_t address) noexcept {
		Bucket& bucket = bucketFor(address);
		bucket.lock.lock();

		std::size_t* link = &bucket.queueHead;
		std::size_t previousIndex = TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX;
		std::size_t currentIndex = bucket.queueHead;

		while (currentIndex != TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX) {
			ParkingLotSlot& current = m_slots[currentIndex];

			if (current.key.load(std::memory_order_relaxed) != address) {
				link = &current.nextInQueue;
				previousIndex = currentIndex;
				currentIndex = *link;
				continue;
			}

			*link = current.nextInQueue;

			if (currentIndex == bucket.queueTail) {
				bucket.queueTail = previousIndex;
			}

			current.nextInQueue = TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX;
			current.key.store(0, std::memory_order_relaxed);
			current.parker.beginUnpark().wake();
			bucket.lock.unlock();
			return true;
		}

		bucket.lock.unlock();
		return false;
	}

	template <typename Callback>
	void unparkOne(std::uintptr_t address, Callback&& callback) noexcept {
		Bucket& bucket = bucketFor(address);
		bucket.lock.lock();

		std::size_t* link = &bucket.queueHead;
		std::size_t previousIndex = TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX;
		std::size_t currentIndex = bucket.queueHead;

		while (currentIndex != TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX) {
			ParkingLotSlot& current = m_slots[currentIndex];

			if (current.key.load(std::memory_order_relaxed) != address) {
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
				while (scanIndex != TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX) {
					const ParkingLotSlot& scan = m_slots[scanIndex];
					if (scan.key.load(std::memory_order_relaxed) == address) {
						hasMoreWaiters = true;
						break;
					}
					scanIndex = scan.nextInQueue;
				}
			}

			current.nextInQueue = TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX;
			current.key.store(0, std::memory_order_relaxed);

			std::forward<Callback>(callback)(UnparkOneResult{.woke = true, .hasMoreWaiters = hasMoreWaiters});

			current.parker.beginUnpark().wake();
			bucket.lock.unlock();
			return;
		}

		std::forward<Callback>(callback)(UnparkOneResult{.woke = false, .hasMoreWaiters = false});
		bucket.lock.unlock();
	}

	void unparkAll(std::uintptr_t address) noexcept {
		Bucket& bucket = bucketFor(address);
		bucket.lock.lock();

		std::size_t* link = &bucket.queueHead;
		std::size_t previousIndex = TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX;
		std::size_t currentIndex = bucket.queueHead;

		while (currentIndex != TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX) {
			ParkingLotSlot& current = m_slots[currentIndex];

			if (current.key.load(std::memory_order_relaxed) != address) {
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

			current.nextInQueue = TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX;
			current.key.store(0, std::memory_order_relaxed);
			current.parker.beginUnpark().wake();

			currentIndex = kNextIndex;
		}

		bucket.lock.unlock();
	}

	template <typename ShouldRequeue>
	[[nodiscard]] bool unparkOneRequeue(std::uintptr_t fromAddress,
	                                    std::uintptr_t toAddress,
	                                    ShouldRequeue&& shouldRequeue) noexcept {
		Bucket& fromBucket = bucketFor(fromAddress);
		Bucket& toBucket = bucketFor(toAddress);
		const bool kSameBucket = (&fromBucket == &toBucket);

		fromBucket.lock.lock();
		if (!kSameBucket) {
			toBucket.lock.lock();
		}

		const bool kRequeue = std::forward<ShouldRequeue>(shouldRequeue)();

		std::size_t* link = &fromBucket.queueHead;
		std::size_t previousIndex = TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX;
		std::size_t currentIndex = fromBucket.queueHead;

		while (currentIndex != TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX) {
			ParkingLotSlot& current = m_slots[currentIndex];

			if (current.key.load(std::memory_order_relaxed) != fromAddress) {
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
				current.key.store(toAddress, std::memory_order_relaxed);
				pushToQueue(toBucket, currentIndex);
			} else {
				current.nextInQueue = TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX;
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
	[[nodiscard]] UnparkAllRequeueResult unparkAllRequeue(std::uintptr_t fromAddress,
	                                                      std::uintptr_t toAddress,
	                                                      ShouldRequeue&& shouldRequeue) noexcept {
		Bucket& fromBucket = bucketFor(fromAddress);
		Bucket& toBucket = bucketFor(toAddress);
		const bool kSameBucket = (&fromBucket == &toBucket);

		fromBucket.lock.lock();
		if (!kSameBucket) {
			toBucket.lock.lock();
		}

		const bool kRequeueAll = std::forward<ShouldRequeue>(shouldRequeue)();

		bool handledAny = false;
		bool anyRequeued = false;
		std::size_t* link = &fromBucket.queueHead;
		std::size_t previousIndex = TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX;
		std::size_t currentIndex = fromBucket.queueHead;

		while (currentIndex != TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX) {
			ParkingLotSlot& current = m_slots[currentIndex];

			if (current.key.load(std::memory_order_relaxed) != fromAddress) {
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
				current.nextInQueue = TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX;
				current.key.store(0, std::memory_order_relaxed);
				current.parker.beginUnpark().wake();
			} else {
				current.key.store(toAddress, std::memory_order_relaxed);
				if (!kSameBucket) {
					pushToQueue(toBucket, currentIndex);
				}
				anyRequeued = true;
			}

			currentIndex = kNextIndex;
		}

		if (!kSameBucket) {
			toBucket.lock.unlock();
		}
		fromBucket.lock.unlock();

		return UnparkAllRequeueResult{.handledAny = handledAny, .anyRequeued = anyRequeued};
	}

private:
	[[nodiscard]] TRIVIAL_FORCE_INLINE static std::size_t currentSlotIndex() noexcept {
		const trivial::thread::Thread* const kCurrent = trivial::thread::Thread::current();
		TRIVIAL_ASSERT(kCurrent != nullptr);

		return kCurrent->index();
	}

	[[nodiscard]] TRIVIAL_FORCE_INLINE Bucket& bucketFor(std::uintptr_t address) noexcept TRIVIAL_LIFETIMEBOUND {
		return m_buckets[hash::fibonacciHash(address, m_bucketBits)];
	}

	[[nodiscard]] static std::size_t bucketCountFor(std::size_t capacity) noexcept {
		return std::bit_ceil(capacity * TRIVIAL_SYNC_PARKING_LOT_LOAD_FACTOR);
	}

	void pushToQueue(Bucket& bucket, std::size_t slotIndex) noexcept {
		m_slots[slotIndex].nextInQueue = TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX;

		if (bucket.queueTail == TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX) {
			bucket.queueHead = slotIndex;
		} else {
			m_slots[bucket.queueTail].nextInQueue = slotIndex;
		}

		bucket.queueTail = slotIndex;
	}

	[[nodiscard]] bool removeFromQueue(Bucket& bucket, std::size_t slotIndex) noexcept {
		std::size_t* link = &bucket.queueHead;
		std::size_t previousIndex = TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX;
		std::size_t currentIndex = bucket.queueHead;

		while (currentIndex != TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX) {
			if (currentIndex != slotIndex) {
				link = &m_slots[currentIndex].nextInQueue;
				previousIndex = currentIndex;
				currentIndex = *link;
				continue;
			}

			const std::size_t kNextIndex = m_slots[currentIndex].nextInQueue;
			*link = kNextIndex;

			if (currentIndex == bucket.queueTail) {
				bucket.queueTail = previousIndex;
			}

			m_slots[currentIndex].nextInQueue = TRIVIAL_SYNC_PARKING_LOT_INVALID_SLOT_INDEX;
			return true;
		}

		return false;
	}

	trivial::core::HeapArray<ParkingLotSlot> m_slots;
	trivial::core::HeapArray<Bucket> m_buckets;
	std::uint32_t m_bucketBits;
};

namespace detail {

inline constinit ParkingLot* g_activeParkingLot = nullptr; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

} // namespace detail

inline void setActiveParkingLot(ParkingLot* const kParkingLot) noexcept {
	detail::g_activeParkingLot = kParkingLot;
}

[[nodiscard]] inline ParkingLot& activeParkingLot() noexcept {
	TRIVIAL_ASSERT(detail::g_activeParkingLot != nullptr);
	return *detail::g_activeParkingLot;
}

template <typename Object>
[[nodiscard]] TRIVIAL_FORCE_INLINE std::uintptr_t parkingKey(Object& object) noexcept {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	return reinterpret_cast<std::uintptr_t>(std::addressof(object));
}

} // namespace trivial::sync

#endif // TRIVIAL_SRC_CORE_SYNC_PARKING_LOT_H
