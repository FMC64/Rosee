#include <iostream>
#include "Rosee/Map.hpp"
#include "Rosee/Renderer.hpp"
#include <chrono>
#include <thread>
#include <mutex>
#include <random>
#include <ctime>
#include <cmath>

#ifdef DEBUG
static inline constexpr bool is_debug = true;
#else
static inline constexpr bool is_debug = false;
#endif

using namespace Rosee;

class Game
{
	Renderer m_r;
	Map m_m;
	std::mutex m_done_mtx;
	bool m_done = false;
	std::thread m_rt;

	void render(void)
	{
		std::mt19937_64 m_rand_gen(time(nullptr));
		auto zrand = [&](void) -> double {
			return static_cast<double>(m_rand_gen()) / static_cast<double>(std::numeric_limits<decltype(m_rand_gen())>::max());
		};
		auto nrand = [&](void) -> double {
			return zrand() * 2.0 - 1.0;
		};

		size_t p_count = 512000;
		auto [b, n] = m_m.addBrush<Id, Point2D, OpaqueRender>(p_count);
		auto points = b.get<Point2D>();
		auto render = b.get<OpaqueRender>();
		auto id = *b.get<Id>();
		for (size_t i = 0; i < p_count; i++) {
			auto &p = points[n + i];
			p.color = glm::vec3(zrand(), zrand(), zrand());
			p.pos = glm::vec2(nrand(), nrand());
			p.base_pos = p.pos;
			p.size = zrand() * 16.0f;
			render[i].pipeline = m_r.pipeline_particle;
			render[i].material = nullptr;
			render[i].model = m_r.model_point;
		}
		auto bef = std::chrono::high_resolution_clock::now();
		double t = 0.0;
		while (true) {
			{
				std::lock_guard l(m_done_mtx);
				if (m_done)
					break;
			}
			m_r.resetFrame();
			auto now = std::chrono::high_resolution_clock::now();
			auto delta = static_cast<std::chrono::duration<double>>(now - bef).count();
			bef = now;
			t += delta;
			auto st = std::sin(t);
			auto [b, n] = m_m.find(id);
			if (b) {
				auto points = b->get<Point2D>();
				for (size_t i = 0; i < p_count; i++) {
					auto &p = points[n + i];
					p.pos = glm::vec2(p.base_pos.x * st, p.base_pos.y * st);
				}
			}
			m_r.render(m_m);
		}
	}

public:
	Game(void) :
		m_r(3, is_debug, false),
		m_rt([this](){
			render();
		})
	{
	}
	~Game(void)
	{
		m_rt.join();
	}

	void run(void)
	{
		while (true) {
			m_r.pollEvents();
			if (m_r.shouldClose()) {
				std::lock_guard l(m_done_mtx);
				m_done = true;
				break;
			}
		}
	}
};

int main(void)
{
	auto g = Game();
	g.run();
	return 0;
}