#ifndef TRIVIAL_TASK_TASK_PAYLOAD_H
#define TRIVIAL_TASK_TASK_PAYLOAD_H

// TODO: For trivially destructible callables and suitable result types,
// avoid the intermediate object and write the returned value directly
// into the reused payload storage.

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include <trivial/core/assert.h>

namespace trivial::task {

// Invoking a moved-from TaskPayload, or one constructed from a null function
// pointer or an empty callable wrapper, results in undefined behavior
class TaskPayload { // NOLINT(cppcoreguidelines-pro-type-member-init)
public:
	TaskPayload() noexcept = delete;

	template <typename Callable>
	    requires(!std::is_same_v<std::remove_cvref_t<Callable>, TaskPayload>
	             && std::is_nothrow_constructible_v<std::decay_t<Callable>, Callable &&>
	             && std::is_nothrow_invocable_v<std::decay_t<Callable>&>
	             && std::is_same_v<std::invoke_result_t<std::decay_t<Callable>&>, void>
	             && std::is_nothrow_destructible_v<std::decay_t<Callable>>)
	TaskPayload(Callable&& callable) noexcept { // NOLINT(cppcoreguidelines-pro-type-member-init)
		using StoredCallable = std::decay_t<Callable>;

		if constexpr (kCanStoreInline<StoredCallable>) {
			std::construct_at(rawStoragePointer<StoredCallable>(m_storage.data()), std::forward<Callable>(callable));

			m_operations = &getInlineOperations<StoredCallable>();
		} else {
			StoredCallable* object = new StoredCallable(std::forward<Callable>(callable)); // TODO: Custom allocator

			std::construct_at(rawStoragePointer<StoredCallable*>(m_storage.data()), object);

			m_operations = &getHeapOperations<StoredCallable>();
		}
	}

	~TaskPayload() noexcept { reset(); }

	TaskPayload(const TaskPayload&) = delete;
	TaskPayload& operator=(const TaskPayload&) = delete;

	TaskPayload(TaskPayload&& other) noexcept { moveFrom(other); } // NOLINT(cppcoreguidelines-pro-type-member-init)
	TaskPayload& operator=(TaskPayload&& other) noexcept {
		if (this != &other) {
			reset();
			moveFrom(other);
		}

		return *this;
	}

	void operator()() noexcept {
		TRIVIAL_ASSERT(m_operations != nullptr);

		m_operations->invoke(m_storage.data());
	}

private:
	static constexpr std::size_t kInlineStorageSize = 40; // TODO: Profile and adjust
	static constexpr std::size_t kInlineStorageAlignment = alignof(std::max_align_t);

	struct Operations {
		void (*invoke)(std::byte* storage) noexcept;
		void (*move)(std::byte* destination, std::byte* source) noexcept;
		void (*destroy)(std::byte* storage) noexcept;
	};

	template <typename Callable>
	static constexpr bool kCanStoreInline
	    = sizeof(Callable) <= kInlineStorageSize && alignof(Callable) <= kInlineStorageAlignment
	      && std::is_nothrow_move_constructible_v<Callable>;

	template <typename Object>
	[[nodiscard]]
	static Object* rawStoragePointer(std::byte* storage) noexcept {
		return reinterpret_cast<Object*>(storage); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	}

	template <typename Object>
	[[nodiscard]]
	static Object* getStoredObject(std::byte* storage) noexcept {
		return std::launder(rawStoragePointer<Object>(storage));
	}

	template <typename Callable>
	[[nodiscard]]
	static Callable* getInlineObject(std::byte* storage) noexcept {
		return getStoredObject<Callable>(storage);
	}

	template <typename Callable>
	[[nodiscard]]
	static Callable** getHeapPointerSlot(std::byte* storage) noexcept {
		static_assert(sizeof(Callable*) <= kInlineStorageSize);
		static_assert(alignof(Callable*) <= kInlineStorageAlignment);

		return getStoredObject<Callable*>(storage);
	}

	template <typename Callable>
	static const Operations& getInlineOperations() noexcept {
		static constexpr Operations s_kOperations
		    = {[](std::byte* storage) noexcept {
			       std::invoke(*getInlineObject<Callable>(storage));
		       },

		       [](std::byte* destination, std::byte* source) noexcept {
			       Callable* sourceCallable = getInlineObject<Callable>(source);

			       std::construct_at(rawStoragePointer<Callable>(destination), std::move(*sourceCallable));

			       std::destroy_at(sourceCallable);
		       },

		       [](std::byte* storage) noexcept {
			       std::destroy_at(getInlineObject<Callable>(storage));
		       }};

		return s_kOperations;
	}

	template <typename Callable>
	static const Operations& getHeapOperations() noexcept {
		static constexpr Operations s_kOperations
		    = {[](std::byte* storage) noexcept {
			       Callable* object = *getHeapPointerSlot<Callable>(storage);

			       std::invoke(*object);
		       },

		       [](std::byte* destination, std::byte* source) noexcept {
			       Callable** sourceSlot = getHeapPointerSlot<Callable>(source);

			       Callable* object = *sourceSlot;

			       std::construct_at(rawStoragePointer<Callable*>(destination), object);

			       std::destroy_at(sourceSlot);
		       },

		       [](std::byte* storage) noexcept {
			       Callable** slot = getHeapPointerSlot<Callable>(storage);

			       Callable* object = *slot;

			       std::destroy_at(slot);
			       delete object; // TODO: Custom allocator
		       }};

		return s_kOperations;
	}

	void reset() noexcept {
		if (m_operations == nullptr) {
			return;
		}

		const Operations* operations = m_operations;
		m_operations = nullptr;

		operations->destroy(m_storage.data());
	}

	void moveFrom(TaskPayload& other) noexcept {
		if (other.m_operations == nullptr) {
			return;
		}

		const Operations* operations = other.m_operations;
		other.m_operations = nullptr;

		operations->move(m_storage.data(), other.m_storage.data());

		m_operations = operations;
	}

	alignas(kInlineStorageAlignment) std::array<std::byte, kInlineStorageSize> m_storage;
	const Operations* m_operations = nullptr;
};

// NOTE: If later caring about support for different pointer sizes will need adjustment
static_assert(sizeof(TaskPayload) == 48);
static_assert(alignof(TaskPayload) == alignof(std::max_align_t));

static_assert(std::is_nothrow_move_constructible_v<TaskPayload>);
static_assert(std::is_nothrow_move_assignable_v<TaskPayload>);
static_assert(std::is_nothrow_destructible_v<TaskPayload>);

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_PAYLOAD_H
