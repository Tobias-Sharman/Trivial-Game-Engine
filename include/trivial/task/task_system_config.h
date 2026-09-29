#ifndef TRIVIAL_TASK_TASK_SYSTEM_CONFIG_H
#define TRIVIAL_TASK_TASK_SYSTEM_CONFIG_H

#include <trivial/core/config.h> // IWYU pragma: keep

// Match task priority ordering if changing - see task_launch_options.h
#ifndef TRIVIAL_TASK_PRIORITY_WEIGHT_BACKGROUND
#define TRIVIAL_TASK_PRIORITY_WEIGHT_BACKGROUND 1
#endif

#ifndef TRIVIAL_TASK_PRIORITY_WEIGHT_NORMAL
#define TRIVIAL_TASK_PRIORITY_WEIGHT_NORMAL 2
#endif

#ifndef TRIVIAL_TASK_PRIORITY_WEIGHT_HIGH
#define TRIVIAL_TASK_PRIORITY_WEIGHT_HIGH 4
#endif

#ifndef TRIVIAL_TASK_PRIORITY_WEIGHT_CRITICAL
#define TRIVIAL_TASK_PRIORITY_WEIGHT_CRITICAL 8
#endif

#ifndef TRIVIAL_TASK_BATCH_SIZE
#define TRIVIAL_TASK_BATCH_SIZE 15
#endif

static_assert(TRIVIAL_TASK_PRIORITY_WEIGHT_BACKGROUND > 0 && TRIVIAL_TASK_PRIORITY_WEIGHT_NORMAL > 0
                  && TRIVIAL_TASK_PRIORITY_WEIGHT_HIGH > 0 && TRIVIAL_TASK_PRIORITY_WEIGHT_CRITICAL > 0,
              "Every task priority weight must be non-zero");

static_assert(TRIVIAL_TASK_BATCH_SIZE > 0, "Task batch size must be non-zero");

static_assert(TRIVIAL_TASK_BATCH_SIZE >= TRIVIAL_TASK_PRIORITY_WEIGHT_BACKGROUND + TRIVIAL_TASK_PRIORITY_WEIGHT_NORMAL
                                             + TRIVIAL_TASK_PRIORITY_WEIGHT_HIGH
                                             + TRIVIAL_TASK_PRIORITY_WEIGHT_CRITICAL,
              "Task batch size must be at least the sum of the priority weights");

#endif // TRIVIAL_TASK_TASK_SYSTEM_CONFIG_H
