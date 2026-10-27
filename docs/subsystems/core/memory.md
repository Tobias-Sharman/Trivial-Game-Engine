# Memory

This covers the memory capabilities probed at start-up. The allocators built on
them are not documented here yet.

## Capabilities

`trivial/core/memory/memory.h` gives:

- `pageSize()`: the smallest unit memory is committed, freed or protected in
- `allocationGranularity()`: the step address space is reserved in
- `largePageSize()`: only with `TRIVIAL_MEMORY_ENABLE_LARGE_PAGES`, 0 when large
pages are not available

## Known at compile time or probed

Where a platform has one page size it is fixed at compile time
(`TRIVIAL_PLATFORM_PAGE_SIZE_KNOWN` in `platform.h`), so `pageSize()` and
`allocationGranularity()` are constants and nothing is stored:

|                 | Page size                  | Allocation granularity |
| --------------- | -------------------------- | ---------------------- |
| Windows         | 4 KiB                      | 64 KiB                 |
| macOS ARM64     | 16 KiB                     | page size              |
| macOS x86-64    | 4 KiB                      | page size              |
| Linux x86-64    | 4 KiB                      | page size              |
| Linux ARM64     | probed (4, 16 or 64 KiB)   | page size              |

Linux ARM64 kernels can be built with any of the three page sizes, so there the
values are probed with `sysconf(_SC_PAGESIZE)` and stored in
`trivial::memory::detail::g_detectedCapabilities`. The global only exists when
something is probed, so other platforms carry no state for it.

In builds with asserts the OS is still asked on every platform, and start-up
asserts that it agrees with the compile time values, so a wrong assumption is
caught on the first run rather than as a misaligned reservation later. If you
run into such an assertion please raise an issue so it can be addressed.

## Large pages

Large pages are opt-in (`TRIVIAL_MEMORY_ENABLE_LARGE_PAGES`) and untested, see
[platform support](../../manual/platform_support.md).

- Windows: `GetLargePageMinimum`, which is only usable once the user has been
granted the "Lock pages in memory" privilege
- Linux: the size one page table level up from the base page (2 MiB with 4 KiB
pages), as transparent huge pages are a hint the kernel may ignore
- macOS: not available, so the size is 0

## Other start-up checks

Lazy decommit on Linux (`TRIVIAL_MEMORY_PREFER_LAZY_DECOMMIT`) relies on
`MADV_FREE`, which older kernels declare but do not implement before Linux 4.5.
Builds with asserts try it once on a scratch page at start-up.
