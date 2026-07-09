#ifndef TRIVIAL_TASK_TASK_SLOT_MUTEX_H
#define TRIVIAL_TASK_TASK_SLOT_MUTEX_H

#include <mutex>

namespace trivial::task {

// TODO: Make a mutex that is smaller size than std::mutex to reduce page sizes
using TaskSlotMutex = std::mutex;

// TODO: Not as much of a priority since size os fine but if making one might as well make the other
using TaskGraphMutex = std::mutex;

} // namespace trivial::task

#endif // TRIVIAL_TASK_TASK_SLOT_MUTEX_H
