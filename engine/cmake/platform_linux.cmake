target_sources(trivial
	PRIVATE
		src/core/cpu/cpu_linux.cpp
		src/core/cpu/cpu_probe_linux.cpp
		src/core/memory/segment_allocator_metadata_linux.cpp
		src/core/memory/virtual_memory_linux.cpp
		src/core/sync/parker_linux.cpp
		src/core/thread/thread_linux.cpp
		src/core/thread/thread_stack_allocator.cpp
		src/core/time/time_linux.cpp
)

if(TRIVIAL_TARGET_ARCH STREQUAL "arm64")
	target_sources(trivial PRIVATE src/core/cpu/cpu_probe_linux_arm64.cpp)
endif()
