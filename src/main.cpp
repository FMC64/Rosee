#include <iostream>
#include "Rosee/Map.hpp"
#include "Rosee/Renderer.hpp"
#include <chrono>
#include <thread>
#include <mutex>

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
		while (true) {
			{
				std::lock_guard l(m_done_mtx);
				if (m_done)
					break;
			}
			m_r.render();
		}
	}

public:
	Game(void) :
		m_r(3, true, false),
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