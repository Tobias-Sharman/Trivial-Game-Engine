#include <cstdio>
#include <memory>
#include <vector>

#include <trivial/engine.h>

#include "boids/boids_simulation.h"

namespace {

class BoidsLayer final : public trivial::Layer {
public:
	void onStart(trivial::gpu::Context* gpu) noexcept override { m_simulation.init(gpu, boids::g_kInitialBoidCount); }

	void onUpdate(const trivial::FrameContext& frameContext) noexcept override {
		m_simulation.update(static_cast<float>(frameContext.deltaTime));

		std::printf("\033[2J\033[H"); // NOLINT(cppcoreguidelines-pro-type-vararg)
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
		std::printf("Frame %.1f fps - %zu boids\n", 1.0 / frameContext.deltaTime, m_simulation.boidCount());
		(void)std::fflush(stdout);
	}

	[[nodiscard]] std::vector<trivial::render::Drawable> collectDrawables() const noexcept override {
		return m_simulation.drawables();
	}

private:
	boids::BoidsSimulation m_simulation;
};

class DebugLayer final : public trivial::Layer {
public:
	[[nodiscard]] std::vector<trivial::render::Drawable> collectDrawables() const noexcept override { return {}; }
};

} // namespace

int main() {
	trivial::EngineConfig config{};
	config.window.size = {.height = 500, .width = 500}; // NOLINT(readability-magic-numbers)
	config.window.title = "Boids";

	trivial::Engine engine(&config);

	trivial::Application game{std::make_unique<BoidsLayer>()};
	TRIVIAL_ATTACH_DEBUG_LAYER(game, std::make_unique<DebugLayer>());

	engine.run(game);
}
