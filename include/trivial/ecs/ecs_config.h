#ifndef TRIVIAL_ECS_ECS_CONFIG_H
#define TRIVIAL_ECS_ECS_CONFIG_H

#include <cstdint> // IWYU pragma: keep

#include <trivial/core/config.h> // IWYU pragma: keep

// TODO: Decide what is the best split
#ifndef TRIVIAL_ECS_ENTITY_INDEX_BITS
#define TRIVIAL_ECS_ENTITY_INDEX_BITS 24U
#endif

#ifndef TRIVIAL_ECS_ENTITY_GENERATION_BITS
#define TRIVIAL_ECS_ENTITY_GENERATION_BITS 8U
#endif

static_assert(TRIVIAL_ECS_ENTITY_INDEX_BITS > 0, "Entity index bits must be non-zero");
static_assert(TRIVIAL_ECS_ENTITY_GENERATION_BITS > 0, "Entity generation bits must be non-zero");

#define TRIVIAL_ECS_ENTITY_MAX_GENERATION ((std::uint32_t{1} << TRIVIAL_ECS_ENTITY_GENERATION_BITS) - std::uint32_t{1})
#define TRIVIAL_ECS_ENTITY_INVALID_INDEX ((std::uint32_t{1} << TRIVIAL_ECS_ENTITY_INDEX_BITS) - std::uint32_t{1})

#endif // TRIVIAL_ECS_ECS_CONFIG_H
