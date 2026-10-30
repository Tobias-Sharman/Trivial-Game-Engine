#ifndef TRIVIAL_SRC_CORE_CPU_CPU_CAPABILITIES_H
#define TRIVIAL_SRC_CORE_CPU_CPU_CAPABILITIES_H

namespace trivial::cpu {

void initCapabilities() noexcept;
void freeCapabilities() noexcept;

} // namespace trivial::cpu

#endif // TRIVIAL_SRC_CORE_CPU_CPU_CAPABILITIES_H
