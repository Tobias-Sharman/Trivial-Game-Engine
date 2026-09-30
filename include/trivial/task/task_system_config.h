#ifndef TRIVIAL_TASK_TASK_SYSTEM_CONFIG_H
#define TRIVIAL_TASK_TASK_SYSTEM_CONFIG_H

#include <trivial/core/config.h> // IWYU pragma: keep

// Match task priority ordering if changing - see task_launch_options.h
#ifndef TRIVIAL_TASK_PRIORITY_SHARE_BACKGROUND
#define TRIVIAL_TASK_PRIORITY_SHARE_BACKGROUND 1
#endif

#ifndef TRIVIAL_TASK_PRIORITY_SHARE_NORMAL
#define TRIVIAL_TASK_PRIORITY_SHARE_NORMAL 2
#endif

#ifndef TRIVIAL_TASK_PRIORITY_SHARE_HIGH
#define TRIVIAL_TASK_PRIORITY_SHARE_HIGH 4
#endif

#ifndef TRIVIAL_TASK_PRIORITY_SHARE_CRITICAL
#define TRIVIAL_TASK_PRIORITY_SHARE_CRITICAL 8
#endif

#define TRIVIAL_TASK_BATCH_SIZE                                                                                        \
	(TRIVIAL_TASK_PRIORITY_SHARE_BACKGROUND + TRIVIAL_TASK_PRIORITY_SHARE_NORMAL + TRIVIAL_TASK_PRIORITY_SHARE_HIGH    \
	 + TRIVIAL_TASK_PRIORITY_SHARE_CRITICAL)

#ifndef TRIVIAL_TASK_GRAPH_SLOTS_PER_PAGE
#define TRIVIAL_TASK_GRAPH_SLOTS_PER_PAGE 256U
#endif

#ifndef TRIVIAL_TASK_GRAPH_MAX_TASK_COUNT
#define TRIVIAL_TASK_GRAPH_MAX_TASK_COUNT 65'536U
#endif

#define TRIVIAL_TASK_GRAPH_MAX_PAGE_COUNT                                                                              \
	((TRIVIAL_TASK_GRAPH_MAX_TASK_COUNT / TRIVIAL_TASK_GRAPH_SLOTS_PER_PAGE)                                           \
	 + (TRIVIAL_TASK_GRAPH_MAX_TASK_COUNT % TRIVIAL_TASK_GRAPH_SLOTS_PER_PAGE != 0 ? 1U : 0U))

// TODO: Profile and adjust
#ifndef TRIVIAL_TASK_PAYLOAD_INLINE_STORAGE_SIZE
#define TRIVIAL_TASK_PAYLOAD_INLINE_STORAGE_SIZE 40U
#endif

static_assert(TRIVIAL_TASK_PRIORITY_SHARE_BACKGROUND > 0 && TRIVIAL_TASK_PRIORITY_SHARE_NORMAL > 0
                  && TRIVIAL_TASK_PRIORITY_SHARE_HIGH > 0 && TRIVIAL_TASK_PRIORITY_SHARE_CRITICAL > 0,
              "Every task priority share must be non-zero");

static_assert(TRIVIAL_TASK_GRAPH_SLOTS_PER_PAGE > 0, "Task graph slots per page must be non-zero");

static_assert(TRIVIAL_TASK_GRAPH_MAX_TASK_COUNT > 0, "Task graph max task count must be non-zero");

#endif // TRIVIAL_TASK_TASK_SYSTEM_CONFIG_H
