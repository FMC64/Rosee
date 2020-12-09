#include <iostream>
#include "Rosee/Map.hpp"
#include "Rosee/Renderer.hpp"
#include <chrono>
#include <thread>
#include <mutex>
#include <random>
#include <ctime>
#include <cmath>
#include <glm/gtx/transform.hpp>

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

		size_t p_count = 64;
		auto [b, n] = m_m.addBrush<Id, Point2D, OpaqueRender>(p_count);
		auto points = b.get<Point2D>();
		auto render = b.get<OpaqueRender>();
		auto id = *b.get<Id>();

		auto pipeline_pool = PipelinePool(64);
		auto model_pool = ModelPool(64);

		auto pipeline_particle = pipeline_pool.allocate();
		*pipeline_particle = m_r.createPipeline("sha/particle", 0);
		pipeline_particle->pushDynamic<Point2D>();

		auto model_point = model_pool.allocate();
		model_point->primitiveCount = 1;
		model_point->vertexBuffer = m_r.createVertexBuffer(sizeof(glm::vec2));
		model_point->indexType = VK_INDEX_TYPE_NONE_KHR;

		auto model_world = model_pool.allocate();
		*model_world = m_r.loadModel("res/mod/vokselia_spawn.obj");

		for (size_t i = 0; i < p_count; i++) {
			auto &p = points[n + i];
			p.color = glm::vec3(zrand(), zrand(), zrand());
			p.pos = glm::vec2(nrand(), nrand());
			p.base_pos = p.pos;
			p.size = zrand() * 16.0f;
			render[i].pipeline = pipeline_particle;
			render[i].material = nullptr;
			render[i].model = model_point;
		}

		auto pipeline_opaque = pipeline_pool.allocate();
		*pipeline_opaque = m_r.createPipeline3D("sha/opaque", 0);
		pipeline_opaque->pushDynamic<MVP>();

		{
			auto [b, n] = m_m.addBrush<Id, Transform, MVP, OpaqueRender>(1);
			b.get<Transform>()[n] = glm::scale(glm::vec3(100.0f));
			auto &r = b.get<OpaqueRender>()[n];
			r.pipeline = pipeline_opaque;
			r.model = model_world;
		}

		auto bef = std::chrono::high_resolution_clock::now();
		double t = 0.0;
		auto camera_pos = glm::vec3(0.0f);
		m_r.setCursorMode(false);
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
			{
				const double ang_rad = pi / 180.0;

				const double near = 0.1, far = 1000.0,
				ratio = static_cast<double>(m_r.swapchainExtent().width) / static_cast<double>(m_r.swapchainExtent().height),
				fov = 70.0 * ang_rad;
				auto proj = glm::perspectiveLH_ZO<float>(fov, ratio, near, far);
				proj[1][1] *= -1.0;

				//auto view = glm::lookAtLH(glm::vec3(0.0, 0.0, -7.0), glm::vec3(0.0), glm::vec3(0.0, 1.0, 0.0));
				auto cursor = m_r.cursor();
				//if (!disable_cam_rot)
				//	cursor = m_r.cursor() - base_cursor;
				const float sensi = 0.1;
				float move = m_r.keyState(GLFW_KEY_LEFT_SHIFT) ? 20.0 : 2.0;
				auto view_rot = glm::rotate(static_cast<float>(-cursor.x * ang_rad) * sensi, glm::vec3(0.0, 1.0, 0.0));
				view_rot = glm::rotate(static_cast<float>(std::clamp(-cursor.y * sensi, -90.0, 90.0) * ang_rad), glm::vec3(1.0, 0.0, 0.0)) * view_rot;
				auto view_rot_inverse = glm::inverse(view_rot);
				auto dir_fwd_w = view_rot_inverse * glm::vec4(0.0, 0.0, 1.0, 0.0);
				auto dir_fwd = glm::vec3(dir_fwd_w.x, dir_fwd_w.y, dir_fwd_w.z);
				auto dir_side_w = view_rot_inverse * glm::vec4(1.0, 0.0, 0.0, 0.0);
				auto dir_side = glm::vec3(dir_side_w.x, dir_side_w.y, dir_side_w.z);
				if (m_r.keyState(GLFW_KEY_W))
					camera_pos += dir_fwd * move * static_cast<float>(delta);
				if (m_r.keyState(GLFW_KEY_S))
					camera_pos -= dir_fwd * move * static_cast<float>(delta);
				if (m_r.keyState(GLFW_KEY_A))
					camera_pos -= dir_side * move * static_cast<float>(delta);
				if (m_r.keyState(GLFW_KEY_D))
					camera_pos += dir_side * move * static_cast<float>(delta);
				auto view = view_rot * glm::translate(-camera_pos);
				auto vp = proj * view;

				m_m.query<MVP>([&](Brush &b){
					auto size = b.size();
					auto mvp = b.get<MVP>();
					auto trans = b.get<Transform>();
					for (size_t i = 0; i < size; i++)
						mvp[i] = vp * trans[i];
				});

				/*camera.size = glm::vec2(1.0) / glm::vec2(m_instance.swapchain->extent());
				camera.vp = proj * view;
				camera.view = view;
				glm::mat4 view_normal = view;
				camera.view_normal = view;
				for (size_t i = 0; i < 3; i++)
					view_normal[3][i] = 0.0f;
				camera.view_normal = view_normal;
				camera.view_normal_inv = glm::inverse(view_normal);
				camera.proj = proj;
				camera.inv_proj = glm::inverse(proj);
				camera.near = near;
				camera.far = far;
				camera.a = far / (far - near);
				camera.b = -(far * near) / (far - near);
				camera.ratio = glm::vec2(ratio, -1.0) * glm::vec2(std::tan(fov / 2.0));*/
			}
			m_r.render(m_m);
		}

		m_r.waitIdle();
		pipeline_pool.destroy(m_r.device);
		model_pool.destroy(m_r.allocator);
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