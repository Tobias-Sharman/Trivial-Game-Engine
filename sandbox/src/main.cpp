#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>

#include <trivial/engine.h>
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

	constexpr auto kSimulatedWork = std::chrono::milliseconds{5};

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
	bool m_launched = false;

	void onUpdate(const trivial::FrameContext& frameContext) override {
		if (!m_launched) {
			m_launched = true;
			launchFullWave();
		}

		std::printf("\033[2J\033[H");

		std::printf("Frame %llu - %d tasks queued\n\n",
		            static_cast<unsigned long long>(frameContext.frameIndex),
		            g_kTotalTaskLimit);

		printBar("Critical", g_criticalCompleted.load(std::memory_order_relaxed), g_kTotalCritical);
		printBar("High", g_highCompleted.load(std::memory_order_relaxed), g_kTotalHigh);
		printBar("Normal", g_normalCompleted.load(std::memory_order_relaxed), g_kTotalBackground);
		printBar("Background", g_backgroundCompleted.load(std::memory_order_relaxed), g_kTotalNormal);

		(void)std::fflush(stdout);

		std::this_thread::sleep_for(std::chrono::milliseconds{100});
	}
};

class DebugLayer final : public trivial::Layer {
	void onUpdate(const trivial::FrameContext& frameContext) override {}
};

} // namespace

int main() {
	trivial::EngineConfig config{.window.size{.height = 720, .width = 1280}, .window.title{"Trivial Test"}};
	trivial::Engine engine(&config);

	trivial::Application game{std::make_unique<GameLayer>()};
	TRIVIAL_ATTACH_DEBUG_LAYER(game, std::make_unique<DebugLayer>());

	engine.run(game);
}
