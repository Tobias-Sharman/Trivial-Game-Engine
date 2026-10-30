# CPU

> **Warning:** these are AI generated notes, to be run through later for the end
> goal version once the Linux side is in. Treat them primarily as a development
> reference, not as a manual. The contents of this file have not been vetted.

## Overview

The CPU module probes the processor once at start-up and keeps what it finds in
`trivial::cpu::detail::g_detectedCapabilities`, read through the accessors in
`trivial/core/cpu/cpu.h`. It covers:

- Identity: vendor and brand strings, and on x86-64 the family, model, stepping
and AVX10 version
- Instruction set features: one enum per architecture, `X86_64Feature` and
`Arm64Feature`, in `cpu_features_x86_64.h` and `cpu_features_arm64.h`
- Topology: logical processors, physical cores, performance and efficiency
cores, efficiency classes, packages, NUMA nodes, caches and the cache line size
- Whether the process is running under translation (Rosetta, or x86-64
emulation on Windows ARM64)

Only facts that are fixed for the machine are stored. Anything that can change
while running is queried on every call instead, so
`availableLogicalProcessorCount()` (the affinity mask) and
`quotaLogicalProcessorCount()` (container or job object CPU limits) never return
a stale value. Topology is treated as fixed at start-up, so processors being
hot plugged afterwards is not reflected.

## Start-up and lifetime

`cpu::initCapabilities()` runs in the `Engine` constructor after the memory
capabilities and before the time capabilities, and `cpu::freeCapabilities()`
runs in the `Engine` destructor to free the topology tables. The accessors
assert (in builds with asserts) that init has run and that the capabilities have
not been freed.

Init must run before any thread is created. Nothing reads the capabilities from
another thread during init, and some lazily enabled state (see AVX-512 on macOS
below) applies to threads created after it.

## Using features

There are two ways to check a feature:

- `TRIVIAL_CPU_HAS(feature)`: for dispatch. If the build's CPU version already
guarantees the feature it is `true` at compile time, so the branch and the
fallback compile out at every optimisation level and on every compiler.
Otherwise it reads the detected set at runtime.
- `cpu::isDetected(feature)`: always the runtime read of what was detected,
ignoring the build's guarantees.

```cpp
if (TRIVIAL_CPU_HAS(trivial::cpu::X86_64Feature::Avx2)) {
    // AVX2 path, compiled with __attribute__((target("avx2"))) on Clang and GCC
} else {
    // Fallback
}
```

The detected set only ever holds what was probed and verified, target features
(`cpu::isTargetFeature`, the features the build's CPU version guarantees) are
never inserted into it. The target feature list is only used at compile time and
by the floor check.

A detected feature means the feature's own instructions can run. It does not
cover what a compiler may emit when targeting it, which is for the code doing
the dispatch to check:

- LLVM's `avx512f` target also enables AVX2, FMA and F16C, and `vaes`,
`sha512` and others imply AVX2, so a kernel compiled for them should check
those too
- AVX-512 instructions at 128 or 256 bits need `Avx512Vl` as well as their own
group
- The 512-bit EVEX forms of VAES, VPCLMULQDQ and GFNI need `Avx512F` (or
`Avx10V1`) as well as their own flag, and the VEX forms of GFNI need `Avx`
- SVE instructions that also exist in streaming mode only need SME there, so
code running in streaming mode checks `Sme` and the `Sme*`/`Ssve*` features
rather than the `Sve*` ones

## CPU versions and the floor check

Each platform builds at a fixed minimum version, set in `engine/cmake/arch_*.cmake`
and specialised in the platform files:

|         | x86-64     | ARM64                        |
| ------- | ---------- | ---------------------------- |
| Windows | x86-64-v2  | Armv8.0                      |
| Linux   | x86-64-v2  | Armv8.0 with outline atomics |
| macOS   | x86-64-v3  | apple-m1                     |

The compiler may use anything in that version anywhere, so a CPU below it would
crash at some arbitrary point. The floor check (`TRIVIAL_CPU_ENABLE_FLOOR_CHECK`,
on by default, in `cpu_config.h`) compares the detected features with the
target features at start-up and, if any are missing, logs each one (and whether
the process is running translated) then aborts. The probe itself is compiled at
the same version, so it is possible for a far older CPU to crash before the check
runs.

Each version's target features are a named list in its features header
(`kVersion2` and `kVersion3` on x86-64, `kAppleM1` on ARM64), built into a set
at compile time so nothing of them is left in the binary. The CMake version is the source of truth,
and two compile time checks keep the compiler flags in line with it:

- The flags must reach the version: a few of the version's compiler macros are
checked, so a build missing `trivial_apply_cpu_version()` fails
- The flags must not go beyond it: every feature the compiler's predefined
macros say it targets (`__AVX2__`, `__ARM_FEATURE_SHA3` and so on) has to be
in the version's list. Otherwise extra flags (`-march=native`, a stray `-mavx512f`)
would let the compiler use instructions the floor check never verifies. BTI is
not checked, as its instructions are hints that older CPUs run as no-ops, and
MSVC defines few of these macros, so there the check covers less

A cache line check (`TRIVIAL_CPU_ENABLE_CACHE_LINE_CHECK`, on outside release)
warns if the reported L1 data line size differs from
`TRIVIAL_PLATFORM_CACHE_LINE_SIZE`.

## How features are detected

### The model

A feature is only reported when every requirement for running its instructions
holds, taken from the vendors' documentation rather than from what compilers
assume:

1. The CPU reports the feature
2. The OS has enabled the register state its instructions use
3. Every feature its instructions depend on is also reported

Wherever Rust's `std_detect` and LLVM's host detection disagree on a rule, the
documentation decides, and where the documentation leaves it open the stricter
of the two is used. Getting a rule wrong in the strict direction only costs a
slower path, while getting it wrong the other way crashes.

On ARM the architecture also has consistency rules, such as SHA1 always coming
with SHA256 or PMULL with AES. These say what a valid CPU must have, not what an
instruction needs to run, so they are not enforced: enforcing them would only
drop features when a report is inconsistent.

### x86-64

Features come from CPUID on every OS, and each leaf is read once, and only when
the CPU's maximum leaf (and for leaf 7 its maximum subleaf) covers it, as
reading past it returns another leaf's data.

Register state is read from XCR0, following the Intel SDM:

- None: legacy SSE and general purpose instructions, BMI1 and BMI2 included as
they do not use vector registers. SSE state itself is always enabled on x86-64
- XSAVE (XSAVE, XSAVEOPT, XSAVEC): needs OSXSAVE. AMD documents the same rule
- YMM (every VEX vector instruction, VEX.128 included): OSXSAVE and XCR0 bits 1
and 2
- ZMM (every EVEX instruction and the opmask instructions): YMM plus XCR0 bits 5
to 7
- APX: OSXSAVE and XCR0 bit 19

Prerequisite features, where a feature missing one is dropped and logged as a
warning with its missing prerequisites, as on ARM64:

- AVX: FMA, F16C, AVX2, SHA512, SM3, SM4, VPCLMULQDQ, and AMD's XOP and FMA4
(the first ones from the SDM, XOP and FMA4 following LLVM as the AMD manual
available did not cover it)
- AVX512F: every AVX-512 group, and AVX512-FP16 also needs AVX512BW
- VAES and the AVX-VNNI, AVX-IFMA and AVX-NE-CONVERT families only need their
own flag and YMM state, as their opcode tables list nothing else

Special cases:

- Erratum SKL052: 6th generation Pentium and Celeron CPUs report BMI1 and BMI2
without supporting them, so on Intel CPUs without the AVX bit they are not
reported. The CPUID bit is used rather than whether AVX is usable, so an OS
turning AVX off does not also hide BMI
- AVX10 is reported as `Avx10V1` and `Avx10V2` from the version in leaf 24H,
which is only read when the AVX10 bit is set and the maximum leaf covers it.
AVX10 uses the AVX-512 register state, AVX10 CPUs also report the AVX-512
flags, and the new AVX10.2 instructions have no flags of their own. The
discrete AVX10 bits in leaf 24H subleaf 1 are not read yet, see the
[roadmap](../../roadmap.md)
- AVX-512 on macOS: threads start with the AVX-512 state off in XCR0 and the OS
turns it on at a thread's first AVX-512 instruction, so on macOS the AVX-512
state is trusted rather than read from XCR0. That first instruction takes one
trap into the kernel, which is invisible to the program
- PKU is read from OSPKE, which is only set once the OS has enabled protection
keys
- PREFETCHI has a different bit on Intel (leaf 7.1 EDX) and AMD (leaf
8000_0021H EAX), either is accepted
- The hypervisor bit, the hybrid bit, invariant TSC and the fast REP MOVSB,
STOSB and CMPSB family are reported as information, they gate no instructions

### ARM64

Features come from the OS:

- macOS: `hw.optional.arm` sysctls, read in one go through `hw.optional.arm.caps`
from macOS 15, with each named sysctl as the fallback and as a check in debug
builds. Features the SDK has no capability bit for (FP8, FPMR, LUT, FAMINMAX
and SME LUTv2) are always read by name, and macOS 14 lacks some of the sysctls,
which then read as not detected
- Linux: `getauxval(AT_HWCAP)`, `AT_HWCAP2` and `AT_HWCAP3`, with the bit values and
the `AT_HWCAP3` type taken from
the kernel's ABI so the build machine's headers do not matter. A kernel too old
to know a feature does not report it
- Windows: `IsProcessorFeaturePresent` with `PF_ARM_*` values. Many features
have no value and read as not detected: RDM, FHM, FCMA, FRINTTS, FlagM, FlagM2,
SB, SSBS, DIT, DPB, DPB2, BTI, PAuth, MTE, SM3, SM4, RNG, LRCPC2, LRCPC3,
LSE128, LUT, FAMINMAX, FPMR and the FP8 family, CSSC, WFxT, MOPS, HBC, AFP,
RPRES and ECV. FPMR is counted as reported when Windows reports an SME or
streaming FP8 feature, as those cannot work unless the OS saves FPMR. Features
added in recent SDKs are passed by value, so older SDKs and older Windows
versions simply report them as absent

The OS only reports features it has enabled, so the register state for SVE, SME
and FPMR is part of what it reports. On top of that, a feature is only reported
if the features its instructions need are also reported, taken from the Arm
machine readable architecture specification. This applies in every build, as an
inconsistent report (an emulator, a VM or a kernel bug) would otherwise pick a
path that faults, and a dropped feature is logged as a warning with its missing
prerequisites:

- SVE: every non-streaming SVE extension (SVE2, SVE2.1 to 2.3, I8MM, BF16,
EBF16, F32MM, F64MM, F16F32MM, the crypto, AES2 and bit permute extensions,
B16B16, BFSCALE, B16MM, PERM256 and LUT6)
- SME: every SME extension (SME2, SME2.1 to 2.3, F64F64, I16I64, F16F16, B16B16,
LUTv2, LUT6, F8F16, F8F32, TMOP, MOP4 and FA64) and every streaming SVE feature
(the `Ssve*` ones), as their instructions run in streaming mode
- SME2: SME LUTv2, as its instructions use the ZT0 register that SME2 adds
- FPMR: every FP8 feature, the FP8 matrix multiplies included, as FP8
instructions read the FP8 mode register
- MTE: MTE2, and MTE2 for the tag checking modes (MTE3, asynchronous, store
only and the tagged fault address), as checking needs tags to have been set
and the modes only change how checking behaves
- BF16 (and SVE BF16): EBF16 and SVE EBF16, which change how BF16 instructions
behave
- AFP: RPRES, which only applies with AFP's alternate floating point handling

The architecture also requires some features to come together (SVE2.1 implies
SVE2, SME F16F16 implies SME2). Those are not checked, because the instructions
do not need them. Where only some of a feature's instructions need more (the
SME2.1 lookup table instructions use ZT0, the multi-vector SVE B16B16 forms need
SME2), callers check that extra feature themselves

Streaming mode is modelled with separate features, as the Arm specification
does: `Sve*` features mean the instructions run outside streaming mode, and
`Ssve*`, `Sme*` and the SME versions say what runs inside it. A single flag for
both would not say which mode is safe. So macOS's `FEAT_SVE_B16B16` on a chip
with SME but no SVE is not reported, as it would only run in streaming mode and
there is no streaming B16B16 feature to report it as.

The same feature can mean slightly different things between OSes. Linux has no
report for FEAT_MTE on its own, so its MTE report (which means FEAT_MTE2) sets
both `Mte` and `Mte2`.

### Not covered

Features that cannot be missing are left out, as a flag that is always set
tells the caller nothing:

- The x86-64 baseline (SSE, SSE2, MMX, FXSR, CMOV, CX8, TSC) and the ARM64
baseline (FP, AdvSIMD), which the ABI requires and the compiler uses freely
- Features the architecture makes mandatory with one that is covered: SME's
F32F32, B16F32, F16F32 and I8I32 formats come with SME, SME2's I16I32 and
BI32I32 with SME2, and FP8's E4M3 and E5M2 formats with FP8

Features that can never run in the engine's code are left out:

- Supervisor only instructions (XSAVES, WBNOINVD, INVPCID, PCONFIG, HRESET)
- Features no supported OS enables for user code (UINTR, USERMSR)

Features an OS reports but that have no use in the engine are left out for now,
see the [roadmap](../../roadmap.md):

- x86-64: AMX, the Xeon Phi AVX-512 groups (ER, PF, 4VNNIW, 4FMAPS) and Xeon
Phi's ring 3 MONITOR/MWAIT, Key Locker, AMD's LWP, FSGSBASE (which only Linux
enables, so it cannot carry the engine's thread local storage), SGX enclaves (which need an
OS driver), ENQCMD (which needs a device bound PASID), PTWRITE (only useful
while Intel PT tracing) and AMD's RDPRU
- ARM64: the PAuth variants (PAuth2, FPAC, FPACCOMBINE, PACIMP, PAuth_LR) and
CPA, which matter to compilers rather than to engine code; the Spectre
properties (CSV2, CSV3, SPECRES, SPECRES2); MTE4 and its canonical and no
address tag modes; DGH; and LS64, which only works on device memory

Not yet covered:

- Whether CET shadow stacks (x86-64) or GCS (ARM64) are on for the process,
which needs a per OS query (Linux `arch_prctl`, Windows' process mitigation
policy), see the [roadmap](../../roadmap.md)

Other limits:

- A virtual machine or emulator that reports a feature it cannot run will crash
on first use of that feature, see the [roadmap](../../roadmap.md) for graceful
handling
- On x86-64 an OS can stop a process reading the timestamp counter (Linux's
`PR_SET_TSC`, used by rr and some sandboxes), which makes RDTSCP, TPAUSE and
UMWAIT fault even when reported. This can change while running, so it is not
probed

## Changing the feature list

Every feature is named in a fixed set of places, and the compile time checks
fail if one is missed.

Adding a feature:

1. The enum in `cpu_features_x86_64.h` or `cpu_features_arm64.h`, in its group,
with a comment saying what it is for, and its case in `featureName`
2. Where it is detected:
   - x86-64: in `cpu_probe_x86_64.cpp`, a case in `cpuidBit` returning its
   CPUID register and bit (a `TRIVIAL_CPU_X86_64_<FEATURE>_BIT` macro), and a
   case in `featureRegisterState` for the register state it needs. A second
   source (a different bit per vendor, as with PREFETCHI) is a special case in
   `probeFeatures`
   - ARM64: a case in each OS's source switch, which the compiler
   (`-Wswitch-enum`) requires even when the OS has no source for it. macOS
   (`sysctlSource` in `cpu_probe_macos_arm64.cpp`) returns the SDK's capability
   bit, or `TRIVIAL_CPU_MACOS_NO_CAPS_BIT` when there is none, and the sysctl
   name, or a null name when there is no sysctl. Linux (`hwcapSource` in
   `cpu_probe_linux_arm64.cpp`) returns the `getauxval` key of its HWCAP word
   and its bits, named after the kernel's `HWCAP*_` constants, or `AT_NULL` when
   there is none.
   Windows (`processorFeature` in `cpu_probe_windows_arm64.cpp`) returns its
   `PF_ARM_*` value, or `TRIVIAL_CPU_WINDOWS_ARM64_PF_NONE` when there is none
3. Its case in `featurePrerequisites` (in `cpu_probe_x86_64.cpp` on x86-64,
`cpu_probe_arm64.h` on ARM64), with every prerequisite including those of its
prerequisites, from the SDM or the instruction conditions in Arm's
specification. It is a switch over every feature, so the
compiler (`-Wswitch-enum`) fails a feature that has no case, and one with none
goes in the group at the end
4. Its compiler macro in `compilerTargetFeatures`, if compilers define one
5. The target list for a version (`kVersion2` and `kVersion3` on x86-64,
`kAppleM1` on ARM64) if that version guarantees it
6. This page: the prerequisite rules, and the OS notes if a source is missing

Every feature with a CPUID or OS source is included, even ones engine code may
never use, as each costs one bit and a case in each switch. In theory the
checks could be reduced by dropping the features the engine does not use, but
keeping track of that is tedious, so it is not done.

Removing a feature: comment out every one of those places (searching for the
enum name finds them all), so it can be restored by uncommenting. A target
feature cannot be removed while its version is in use, as the floor check needs
it. Removing a feature that others need as a prerequisite means removing it
from their rules too.

## Identity

- Vendor
  - x86-64: the CPUID vendor ID (`GenuineIntel`, `AuthenticAMD`,
  `HygonGenuine` and so on)
  - ARM64: the name of the MIDR implementer code (`ARM`, `Qualcomm`, `Apple`
  and so on), read through the kernel's emulated MIDR_EL1 on Linux and the
  registry's copy of it on Windows, and `Apple` on macOS. Empty for an
  implementer not in the list. The list (`implementerName` in
  `cpu_probe_arm64.h`) only has implementers of 64-bit Arm cores, so
  supporting 32-bit Arm would want the switch extended with the 32-bit era ones
  (DEC `0x44`, Infineon `0x49`, Motorola/Freescale `0x4D`, Faraday `0x66` and
  Intel `0x69`). A `static_assert` checks every name fits the vendor string.
  When the OS does not allow reading MIDR (Linux without `HWCAP_CPUID`, or no
  registry key on Windows) the vendor is empty and an error is logged
- Brand
  - x86-64: the CPUID brand string, with Intel's leading spaces trimmed
  - macOS: `machdep.cpu.brand_string`
  - Windows ARM64: the registry's `ProcessorNameString`
  - Linux ARM64: empty, as the kernel only exposes it through files
  - macOS and Windows ARM64 read the size first and then the name, and a name
  longer than the brand string is cut short with a warning (`copyBrand`)
- x86-64 family, model and stepping follow the SDM's extended family and model
rules

## Topology

### Per platform

- Windows: `GetLogicalProcessorInformationEx(RelationAll)`, read once into a
buffer of the size Windows asks for
  - Each core record gives its logical processors and efficiency class, with
  each logical processor numbered as its group times 64 plus its bit, which is
  what thread pinning uses
  - Package, NUMA node and cache records are matched to logical processors by
  their group masks, with a group count of 0 (older Windows) read as a single
  mask. The masks read are capped to what fits in the record, so a record
  claiming more groups than it holds cannot be read past
  - ARM64 also reads each logical processor's MIDR from the registry
  (`probeMainIds`, gated by `TRIVIAL_CPU_PROBE_HAS_MAIN_IDS` as macOS has no
  per processor MIDR). The registry numbers processors in order, so on machines
  with more than 64 logical processors this relies on that order matching the
  group order. A processor whose MIDR cannot be read has a zero main ID, with
  one error logged
- Linux: not probed yet, as it needs the file reading interface, so the
topology is unknown and logged at start-up, see the
[roadmap](../../roadmap.md). The per processor main IDs also need sysfs file
reads, so `probeMainIds` logs that they are unknown and zeroes them
- macOS: threads cannot be pinned, so only the performance levels are stored as
efficiency classes, from the `hw.perflevel` sysctls, each with its core and
logical processor counts and its caches

### Layout

On platforms with CPU affinity (Windows and Linux) the full topology is stored:
`logicalProcessors()` gives each logical processor's OS processor number, core,
package, NUMA node, efficiency class and cache indices, and `caches()` gives the
caches they point into. Logical processors are sorted by efficiency class
(slowest first), then core, then processor number, so the logical processors of
a core sit next to each other. Core, package and NUMA node numbers are dense
from 0, following the OS's order.

On macOS `efficiencyClasses()` gives one entry per performance level instead.

Efficiency classes are ranked from 0, with a higher class being faster, so the
OS's own values (Windows' efficiency class, macOS's performance levels) are only
used for their order.

### Caches

- Only L1 data, L1 instruction, L2 and L3 caches are kept. Anything else (L4,
or split L2 and L3) is left out with a warning
- The cache line size is the L1 data cache's line size
- Associativity is 0 when unknown, and `TRIVIAL_CPU_CACHE_FULLY_ASSOCIATIVE`
when fully associative
- `sharingLogicalProcessorCount` is how many logical processors share each cache

### When the OS reports something wrong

Missing or contradictory data makes that part unknown rather than stopping
start-up:

- No logical processors, or more than 65535, makes the whole topology unknown
- More caches than can be indexed, or caches that do not match the logical
processors (one a processor uses but the OS did not list, or one the OS listed
that no processor uses), makes the caches unknown
- More than 256 efficiency classes makes the classes and the performance and
efficiency core counts unknown

A count of 0, an empty table or a cache size of 0 means the OS did not report
it.

## Logical processor counts

- `logicalProcessorCount()`: every logical processor in the topology, 0 when it
is unknown
- `availableLogicalProcessorCount()`: the ones this process may run on, queried
on every call
  - Windows: the process affinity mask when the machine has one processor
  group, or when the mask leaves out part of its group and the process is in
  that one group, as that is a deliberate restriction (`start /affinity`, a
  parent's affinity, a job's affinity limit). The one group check is there
  because Windows 11 can report a partial mask for a process spanning several
  groups when the groups differ in size. Otherwise the processors of the groups the job allows
  (job group affinity), or every active processor when the process is not in
  a job. A full single group mask is not a limit on machines with more than
  64 logical processors, as threads can be placed in any group the job allows
  with `SetThreadGroupAffinity`. From Windows 11 they span every group by
  default; on Windows 10 they start in one group, so thread placement has to
  put them in the others. CPU sets are only hints, so they are not counted
  - Linux: `sched_getaffinity`, so `taskset` and cpusets are respected. It
  reads the calling thread's affinity, as Linux has no process wide one, so a
  pinned thread gets a smaller count. An info log says so on each query until
  it is replaced with a start-up read
  - macOS: `hw.activecpu`
- `quotaLogicalProcessorCount()`: the CPU time the process is allowed, as a
number of logical processors, never more than the available count and never
less than 1. A quota of part of a processor is rounded up, or down with
`TRIVIAL_CPU_ROUND_QUOTA_UP` set to 0 in `cpu_config.h`
  - Windows: a job object's CPU rate hard cap or maximum rate.
  Only the process's own job is read: in nested jobs a rate is a share of its
  parent's rate, so a cap set on a parent job is missed and the quota can be
  too high
  - Linux: the same as the available count for now, with an info log each
  time it is queried, as cgroup quotas need the file reading interface
  - macOS: the same as the available count, as macOS has no such quota

## Translation

- macOS: `sysctl.proc_translated`
- Windows: an x86-64 build whose native machine (`IsWow64Process2`) is ARM64.
An ARM64 build cannot be translated
- Linux: always false for now, with an info log, as Linux has no OS report and
translators have to be recognised one by one: FEX, Box64, qemu-user and Rosetta
for Linux run x86-64 builds on ARM64, and qemu-user also runs ARM64 builds on
x86-64 (Docker's multi-architecture builds, for example), see the
[roadmap](../../roadmap.md)

## System info logging

Outside release builds the engine logs the CPU identity, features and topology
at start-up, along with the memory and time capabilities. The sections are
toggled in `log_config.h`.

## Platform notes

- Rosetta (macOS 27 on an M4 Pro) reports SSE4.2, AES, PCLMULQDQ, POPCNT and
LAHF/SAHF but no AVX, MOVBE or BMI, so an x86-64 macOS build, which is
x86-64-v3, fails the floor check under Rosetta
- Windows on ARM64 reports far fewer features than the hardware has, see the
list above
