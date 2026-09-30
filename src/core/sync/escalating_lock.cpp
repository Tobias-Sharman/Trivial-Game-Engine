#define TRIVIAL_SYNC_ESCALATING_LOCK_IMPLEMENTATION
#include "core/sync/escalating_lock.h"

#include <atomic>
#include <cstdint>

#include <trivial/core/compiler.h>
#include <trivial/core/sync/spin_wait.h>
#include <trivial/core/sync/sync_config.h>

#include "core/sync/parker.h"

namespace {

struct EscalatingLockNode {
	trivial::sync::Parker parker;
	EscalatingLockNode* queueTail = nullptr;
	EscalatingLockNode* previous = nullptr;
	EscalatingLockNode* next = nullptr;
};

static_assert(alignof(EscalatingLockNode) > 3, "Low 2 bits of an EscalatingLockNode pointer must be free for tagging");

TRIVIAL_FORCE_INLINE void fenceAcquire(const std::atomic<std::uintptr_t>& state) noexcept {
	// ThreadSanitizer has only partial fence support, so it needs an acquire load here instead
	if constexpr (TRIVIAL_THREAD_SANITIZER_ENABLED) {
		(void)state.load(std::memory_order_acquire);
	} else {
		std::atomic_thread_fence(std::memory_order_acquire);
	}
}

} // namespace

namespace trivial::sync {

void EscalatingLock::lockSlow() noexcept {
	std::uint32_t spinCount = 0;
	std::uintptr_t state = m_state.load(std::memory_order_relaxed);
	thread_local EscalatingLockNode s_node;

	for (;;) {
		if ((state & TRIVIAL_SYNC_ESCALATING_LOCK_LOCKED_BIT) == 0) {
			if (m_state.compare_exchange_weak(state,
			                                  state | TRIVIAL_SYNC_ESCALATING_LOCK_LOCKED_BIT,
			                                  std::memory_order_acquire,
			                                  std::memory_order_relaxed)) {
				return;
			}

			continue;
		}

		if ((state & TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_MASK) == 0 && spinCount < TRIVIAL_SYNC_MAX_SPIN_COUNT) {
			spinWaitForever(spinCount);
			state = m_state.load(std::memory_order_relaxed);
			continue;
		}

		s_node.parker.prepare();

		const std::uintptr_t kQueueHeadAddress = state & TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_MASK;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
		EscalatingLockNode* const kQueueHead = reinterpret_cast<EscalatingLockNode*>(kQueueHeadAddress);
		if (kQueueHead == nullptr) {
			s_node.queueTail = &s_node;
			s_node.previous = nullptr;
		} else {
			s_node.queueTail = nullptr;
			s_node.previous = nullptr;
			s_node.next = kQueueHead;
		}

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		const std::uintptr_t kNodeAddress = reinterpret_cast<std::uintptr_t>(&s_node);
		const std::uintptr_t kNewState = (state & ~TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_MASK) | kNodeAddress;
		if (!m_state.compare_exchange_weak(state, kNewState, std::memory_order_acq_rel, std::memory_order_relaxed)) {
			continue;
		}

		s_node.parker.park();

		spinCount = 0;
		state = m_state.load(std::memory_order_relaxed);
	}
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void EscalatingLock::unlockSlow() noexcept {
	std::uintptr_t state = m_state.load(std::memory_order_relaxed);

	for (;;) {
		if ((state & TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_LOCKED_BIT) != 0
		    || (state & TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_MASK) == 0) {
			return;
		}

		if (m_state.compare_exchange_weak(state,
		                                  state | TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_LOCKED_BIT,
		                                  std::memory_order_acquire,
		                                  std::memory_order_relaxed)) {
			break;
		}
	}

outer:
	for (;;) {
		const std::uintptr_t kQueueHeadAddress = state & TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_MASK;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
		EscalatingLockNode* const kQueueHead = reinterpret_cast<EscalatingLockNode*>(kQueueHeadAddress);
		EscalatingLockNode* queueTail = kQueueHead->queueTail;
		EscalatingLockNode* current = kQueueHead;
		while (queueTail == nullptr) {
			EscalatingLockNode* const kNext = current->next;
			kNext->previous = current;
			current = kNext;
			queueTail = current->queueTail;
		}

		kQueueHead->queueTail = queueTail;

		if ((state & TRIVIAL_SYNC_ESCALATING_LOCK_LOCKED_BIT) != 0) {
			if (m_state.compare_exchange_weak(state,
			                                  state & ~TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_LOCKED_BIT,
			                                  std::memory_order_release,
			                                  std::memory_order_relaxed)) {
				return;
			}

			fenceAcquire(m_state);
			continue;
		}

		EscalatingLockNode* const kNewTail = queueTail->previous;
		if (kNewTail == nullptr) {
			for (;;) {
				if (m_state.compare_exchange_weak(state,
				                                  state & TRIVIAL_SYNC_ESCALATING_LOCK_LOCKED_BIT,
				                                  std::memory_order_release,
				                                  std::memory_order_relaxed)) {
					break;
				}

				if ((state & TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_MASK) == 0) {
					continue;
				}

				fenceAcquire(m_state);
				goto outer; // NOLINT(cppcoreguidelines-avoid-goto)
			}
		} else {
			kQueueHead->queueTail = kNewTail;
			m_state.fetch_and(~TRIVIAL_SYNC_ESCALATING_LOCK_QUEUE_LOCKED_BIT, std::memory_order_release);
		}

		UnparkHandle handle = queueTail->parker.beginUnpark();
		handle.wake();
		return;
	}
}

} // namespace trivial::sync
