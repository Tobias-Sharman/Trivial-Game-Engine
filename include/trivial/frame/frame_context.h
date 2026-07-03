#ifndef TRIVIAL_FRAME_FRAME_CONTEXT_H
#define TRIVIAL_FRAME_FRAME_CONTEXT_H

#include <cstdint>

struct FrameContext {
	double deltaTime = 0.0;
	std::uint64_t frameIndex = 0;
};

#endif // TRIVIAL_FRAME_FRAME_CONTEXT_H
