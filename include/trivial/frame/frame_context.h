#ifndef TRIVIAL_FRAME_FRAME_CONTEXT_H
#define TRIVIAL_FRAME_FRAME_CONTEXT_H

#include <cstdint>

namespace trivial {

struct FrameContext {
	double deltaTime = 0.0;
	std::uint64_t frameIndex = 0;
};

} // namespace trivial
//
#endif // TRIVIAL_FRAME_FRAME_CONTEXT_H
