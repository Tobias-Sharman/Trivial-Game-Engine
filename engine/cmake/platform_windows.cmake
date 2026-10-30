target_sources(trivial
	PRIVATE
		src/core/cpu/cpu_probe_windows.cpp
		src/core/cpu/cpu_windows.cpp
		src/core/memory/segment_allocator_metadata_windows.cpp
		src/core/memory/virtual_memory_windows.cpp
		src/core/sync/parker_windows.cpp
		src/core/thread/thread_windows.cpp
		src/core/time/time_capabilities_windows.cpp
		src/core/time/time_windows.cpp
)

if(TRIVIAL_TARGET_ARCH STREQUAL "arm64")
	target_sources(trivial PRIVATE src/core/cpu/cpu_probe_windows_arm64.cpp)
endif()

target_compile_definitions(trivial PUBLIC _WIN32_WINNT=0x0A00 NTDDI_VERSION=0x0A000005)

target_link_libraries(trivial PRIVATE Synchronization onecore)
