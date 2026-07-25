#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <numbers>
#include <thread>

#include <trivial/engine.h>
#include <trivial/render/shapes.h>
#include <trivial/task/task.h>

namespace {

std::atomic<std::uint64_t> g_backgroundCompleted{0};
std::atomic<std::uint64_t> g_normalCompleted{0};
std::atomic<std::uint64_t> g_highCompleted{0};
std::atomic<std::uint64_t> g_criticalCompleted{0};

constexpr int g_kTotalTaskLimit = 10000;

constexpr int g_kTotalNormal = g_kTotalTaskLimit / 4;
constexpr int g_kTotalHigh = g_kTotalTaskLimit / 4;
constexpr int g_kTotalCritical = g_kTotalTaskLimit / 4;
constexpr int g_kTotalBackground = g_kTotalTaskLimit - g_kTotalNormal - g_kTotalHigh - g_kTotalCritical;

void launchOne(trivial::task::TaskPriority priority, std::atomic<std::uint64_t>& counter) {
	using namespace trivial::task;

	constexpr auto kSimulatedWork = std::chrono::milliseconds{1};

	TaskLaunchOptions options{};
	options.priority = priority;

	auto payload = [&counter, kSimulatedWork]() noexcept {
		std::this_thread::sleep_for(kSimulatedWork);
		counter.fetch_add(1, std::memory_order_relaxed);
	};

	(void)launch(TaskPayload{payload}, options);
}
void launchFullWave() {
	using namespace trivial::task;

	for (int i = 0; i < g_kTotalNormal; ++i) {
		launchOne(TaskPriority::Background, g_backgroundCompleted);
	}

	for (int i = 0; i < g_kTotalBackground; ++i) {
		launchOne(TaskPriority::Normal, g_normalCompleted);
	}

	for (int i = 0; i < g_kTotalHigh; ++i) {
		launchOne(TaskPriority::High, g_highCompleted);
	}

	for (int i = 0; i < g_kTotalCritical; ++i) {
		launchOne(TaskPriority::Critical, g_criticalCompleted);
	}
}

void printBar(const char* label, std::uint64_t completed, int total) {
	constexpr int kBarWidth = 40;

	const int kClampedCompleted
	    = static_cast<int>(std::min<std::uint64_t>(completed, static_cast<std::uint64_t>(total)));
	const int kFilled = total > 0 ? (kClampedCompleted * kBarWidth) / total : 0;
	const int kPercent = total > 0 ? (kClampedCompleted * 100) / total : 0;

	std::printf("%-10s [", label);

	for (int i = 0; i < kBarWidth; ++i) {
		std::putchar(i < kFilled ? '#' : '-');
	}

	std::printf("] %3d%%  (%d/%d)\n", kPercent, kClampedCompleted, total);
}

class GameLayer final : public trivial::Layer {
	void onStart(trivial::gpu::Context* gpu) noexcept override {
		std::array<trivial::rhi::Vertex2, 3> triangleVertices = {{
		    {.position = {.x = 0.0F, .y = -std::numbers::sqrt3_v<float> / 3.0F}, .colour = {255, 0, 0, 255}},
		    {.position = {.x = 0.5F, .y = std::numbers::sqrt3_v<float> / 6.0F}, .colour = {0, 255, 0, 255}},
		    {.position = {.x = -0.5F, .y = std::numbers::sqrt3_v<float> / 6.0F}, .colour = {0, 0, 255, 255}},
		}};
		std::array<std::uint16_t, 3> triangleIndices = {0, 1, 2};

		m_triangleHandle = gpu->createMesh(triangleVertices, triangleIndices);

		const trivial::render::MeshGeometry kQuad = trivial::render::makeQuad();
		m_quadHandle = gpu->createMesh(kQuad.vertices, kQuad.indices);

		const trivial::render::MeshGeometry kCircle = trivial::render::makeCircle(32);
		m_circleHandle = gpu->createMesh(kCircle.vertices, kCircle.indices);
	}

	void onUpdate(const trivial::FrameContext& frameContext) noexcept override {
		m_elapsedSeconds += frameContext.deltaTime;

		// if (!m_launched) {
		// 	m_launched = true;
		// 	launchFullWave();
		// }

		std::printf("\033[2J\033[H");

		std::printf("Frame %f", 1.0F / frameContext.deltaTime);

		// std::printf("Frame %llu - %d tasks queued\n\n",
		//             static_cast<unsigned long long>(frameContext.frameIndex),
		//             g_kTotalTaskLimit);
		//
		// printBar("Critical", g_criticalCompleted.load(std::memory_order_relaxed), g_kTotalCritical);
		// printBar("High", g_highCompleted.load(std::memory_order_relaxed), g_kTotalHigh);
		// printBar("Normal", g_normalCompleted.load(std::memory_order_relaxed), g_kTotalBackground);
		// printBar("Background", g_backgroundCompleted.load(std::memory_order_relaxed), g_kTotalNormal);

		(void)std::fflush(stdout);

		std::this_thread::sleep_for(std::chrono::milliseconds{0});
	}

	[[nodiscard]] std::vector<trivial::render::Drawable> collectDrawables() const noexcept override {
		std::vector<trivial::render::Drawable> drawables;

		constexpr int kCircleCount = 7;
		constexpr float kOrbitRadius = 0.6F;

		for (int i = 0; i < kCircleCount; ++i) {
			const float kPhaseOffset
			    = (2.0F * std::numbers::pi_v<float> * static_cast<float>(i)) / static_cast<float>(kCircleCount);
			const float kOrbitAngle = m_elapsedSeconds + kPhaseOffset;

			const trivial::math::Vec2f kPosition
			    = {.x = kOrbitRadius * std::cos(kOrbitAngle), .y = kOrbitRadius * std::sin(kOrbitAngle)};

			constexpr float kCircleScale = 0.3F;

			drawables.push_back({.mesh = m_circleHandle,
			                     .transform = trivial::math::Affine2f::translation(kPosition)
			                                  * trivial::math::Affine2f::scale(kCircleScale),
			                     .tint = {.x = 1.0F, .y = 0.5F, .z = 0.2F, .w = 1.0F}});
		}

		constexpr int kSquareCount = 2;
		constexpr float kBounceAmplitude = 2.0F;
		constexpr float kBounceSpeed = 3.0F;
		constexpr float kSquareScale = 0.25F;

		const float kOffsetY = kBounceAmplitude * std::sin((m_elapsedSeconds)*kBounceSpeed);

		drawables.push_back(
		    {.mesh = m_quadHandle,
		     .transform = trivial::math::Affine2f::translation({.x = 1.0F - kSquareScale * 0.5F, .y = -kOffsetY})
		                  * trivial::math::Affine2f::scale(kSquareScale),
		     .tint = {.x = 0.2F, .y = 0.6F, .z = 1.0F, .w = 1.0F}});

		drawables.push_back(
		    {.mesh = m_quadHandle,
		     .transform = trivial::math::Affine2f::translation({.x = -1.0F + kSquareScale * 0.5F, .y = kOffsetY})
		                  * trivial::math::Affine2f::scale(kSquareScale),
		     .tint = {.x = 0.2F, .y = 0.6F, .z = 1.0F, .w = 1.0F}});

		const trivial::math::Anglef kAngle = trivial::math::Anglef::fromRadians(6 * m_elapsedSeconds);
		const float kTriangleOffsetY = 0.3F * std::sin(m_elapsedSeconds * 2.0F);
		const trivial::math::Affine2f kTriangleTransform
		    = trivial::math::Affine2f::translation({.x = 0.0F, .y = kTriangleOffsetY})
		      * trivial::math::Affine2f::rotation(kAngle);

		drawables.push_back({.mesh = m_triangleHandle,
		                     .transform = kTriangleTransform,
		                     .tint = {.x = 1.0F, .y = 1.0F, .z = 1.0F, .w = 1.0F}});

		return drawables;
	}

private:
	bool m_launched = false;
	trivial::rhi::MeshHandle m_triangleHandle;
	trivial::rhi::MeshHandle m_quadHandle;
	trivial::rhi::MeshHandle m_circleHandle;
	float m_elapsedSeconds = 0.0F;
};

class DebugLayer final : public trivial::Layer {
	void onUpdate(const trivial::FrameContext& frameContext) noexcept override {}

	[[nodiscard]] std::vector<trivial::render::Drawable> collectDrawables() const noexcept override {
		return std::vector<trivial::render::Drawable>{};
	}
};

} // namespace

int main() {
	trivial::EngineConfig config{.window.size{.height = 720, .width = 1280}, .window.title{"Trivial Test"}};
	trivial::Engine engine(&config);

	trivial::Application game{std::make_unique<GameLayer>()};
	TRIVIAL_ATTACH_DEBUG_LAYER(game, std::make_unique<DebugLayer>());

	engine.run(game);
}
