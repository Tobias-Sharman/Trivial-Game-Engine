target_sources(trivial
	PRIVATE
		src/core/cpu/cpu_macos.cpp
		src/core/cpu/cpu_probe_macos.cpp
		src/core/memory/segment_allocator_metadata_macos.cpp
		src/core/memory/virtual_memory_macos.cpp
		src/core/sync/parker_macos.cpp
		src/core/thread/thread_macos.cpp
		src/core/thread/thread_stack_allocator.cpp
		src/core/time/time_capabilities_macos.cpp
		src/core/time/time_macos.cpp
)

if(TRIVIAL_TARGET_ARCH STREQUAL "x86_64")
	set(TRIVIAL_CPU_X86_64_LEVEL v3)
else()
	set(TRIVIAL_CPU_ARM64_LEVEL apple-m1)

	target_sources(trivial PRIVATE src/core/cpu/cpu_probe_macos_arm64.cpp)
endif()
