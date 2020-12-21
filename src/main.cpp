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
#include "Rosee/c.hpp"

#ifdef DEBUG
static inline constexpr bool is_debug = true;
#else
static inline constexpr bool is_debug = false;
#endif

using namespace Rosee;

class Noise
{
	ivec2 m_seed;
	std::mt19937_64 m_gen;

	double zrand(void)
	{
		return static_cast<double>(m_gen()) / static_cast<double>(std::numeric_limits<decltype(m_gen())>::max());
	}

	glm::dvec2 gradient(const ivec2 &pos)
	{
		auto seq = std::seed_seq{pos.x, pos.y};
		m_gen.seed(seq);
		double ang = zrand() * pi * 2.0;
		return glm::dvec2(std::cos(ang), std::sin(ang));
	}

	double grad_scal(const ivec2 &pos, const glm::dvec2 &dir)
	{
		return glm::dot(gradient(pos), dir);
	}

public:
	Noise(const ivec2 &seed) :
		m_seed(seed)
	{
	}

	double sample(const glm::dvec2 &pos)
	{
		ivec2 ipos(pos);
		auto spos = m_seed + ipos;
		auto lpos = pos - glm::dvec2(ipos);

		//	g0	g1
		//
		//
		//	g2	g3
		auto g0 = grad_scal(spos, -lpos);
		auto g1 = grad_scal(ivec2(spos.x + 1, spos.y), glm::dvec2(1.0 - lpos.x, - lpos.y));
		auto g2 = grad_scal(ivec2(spos.x, spos.y + 1), glm::dvec2(-lpos.x, 1.0 - lpos.y));
		auto g3 = grad_scal(ivec2(spos.x + 1, spos.y + 1), glm::dvec2(1.0 - lpos.x, 1.0 - lpos.y));

		auto g01 = glm::mix(g0, g1, lpos.x);
		auto g23 = glm::mix(g2, g3, lpos.x);
		return glm::mix(g01, g23, lpos.y);
	}
};

class World
{
	Noise m_0;
	Noise m_1;

public:
	static constexpr int64_t chunk_size = 64;

	World(void) :
		m_0(ivec2(0, 0)),
		m_1(ivec2(64000, 25000))
	{
	}

	glm::dvec3 sample(const glm::dvec2 &p)
	{
		auto pos = p + glm::dvec2(0.5);
		auto h = m_0.sample(pos * 0.1) * 3.0 + m_1.sample(pos * 0.0099) * 10.0;
		return glm::vec3(pos.x, h, pos.y);
	}

	glm::dvec3 sample_normal(const glm::dvec2 &pos)
	{
		static constexpr double bias = 0.1;
		auto a = sample(pos);
		auto b = sample(glm::dvec2(pos.x + bias, pos.y + bias * 0.5));
		auto c = sample(glm::dvec2(pos.x + bias * 0.5, pos.y + bias));
		return glm::normalize(glm::cross(b - a, c - a));
	}

	Model createChunk(Renderer &r, const ivec2 &pos, size_t scale)
	{
		int64_t scav = static_cast<int64_t>(1) << scale;
		auto start = ivec2(pos.x * scav * chunk_size, pos.y * scav * chunk_size);
		static constexpr size_t vert_count = chunk_size * chunk_size;
		Vertex::pn vertices[vert_count];
		for (size_t i = 0; i < chunk_size; i++)
			for (size_t j = 0; j < chunk_size; j++) {
				auto &cur = vertices[i * chunk_size + j];
				auto p2d = glm::dvec2(start + ivec2(j * scav, i * scav));
				cur.p = glm::dvec3(j * scav, sample(p2d).y, i * scav);
				cur.n = -sample_normal(p2d);
			}
		static constexpr size_t ind_stride = chunk_size * 2 + 1;
		static constexpr size_t ind_count = ind_stride * (chunk_size - 1);
		uint16_t indices[ind_count];
		std::memset(indices, 0xFF, ind_count * sizeof(uint16_t));
		for (int64_t i = 0; i < (chunk_size - 1); i++) {
			for (int64_t j = 0; j < (chunk_size - 1); j++) {
				indices[ind_stride * i + j * 2] = i * chunk_size + j;
				indices[ind_stride * i + j * 2 + 1] = (i + 1) * chunk_size + j + 1;
			}
		}

		Model res;
		res.primitiveCount = ind_count;
		size_t buf_size = vert_count * sizeof(Vertex::pn);
		size_t ind_size = ind_count * sizeof(uint16_t);
		res.vertexBuffer = r.createVertexBuffer(buf_size);
		res.indexBuffer = r.createIndexBuffer(ind_size);
		res.indexType = VK_INDEX_TYPE_UINT16;
		r.loadBuffer(res.vertexBuffer, buf_size, vertices);
		r.loadBuffer(res.indexBuffer, ind_size, indices);
		return res;
	}
};

class Game
{
	Renderer m_r;
	Map m_m;
	World m_w;

	static int64_t next_chunk_size_n(int64_t val)
	{
		int64_t res = val / 2;
		while (res * 2 + 2 >= val)
			res--;
		return res;
	}

	static int64_t next_chunk_size_p(int64_t val)
	{
		int64_t res = val / 2;
		while (res * 2 < val)
			res++;
		return res;
	}

	void gen_chunks(Pipeline *pipeline, Material *material)
	{
		auto pos = ivec2(0);
		auto pos_end = ivec2(1);
		size_t scale = 0;

		for (size_t i = 0; i < 1; i++) {
			auto npos = ivec2(next_chunk_size_n(pos.x), next_chunk_size_n(pos.y));
			auto npos_end = ivec2(next_chunk_size_p(pos_end.x), next_chunk_size_p(pos_end.y));
			auto nscale = scale + 1;
			int64_t scav = static_cast<int64_t>(1) << nscale;

			/*std::cout << "npos: " << npos.x << ", " << npos.y << std::endl;
			std::cout << "npos_end: " << npos_end.x << ", " << npos_end.y << std::endl;
			std::cout << "scav: " << scav << std::endl;*/

			for (int64_t j = npos.y; j < npos_end.y; j++)
				for (int64_t k = npos.x; k < npos_end.x; k++) {
					auto cpos = ivec2(k, j);
					auto cpos_sca = cpos * static_cast<int64_t>(2);
					if (!(cpos_sca.x >= pos.x && cpos_sca.y >= pos.y && cpos_sca.x < pos_end.y && cpos_sca.y < pos_end.y)) {
						auto model = new Model(m_w.createChunk(m_r, cpos, nscale));
						auto [b, n] = m_m.addBrush<Id, Transform, MVP, MV_normal, MW_local, OpaqueRender>(1);
						auto off = glm::dvec3(cpos.x * scav * World::chunk_size, 0.0, cpos.y * scav * World::chunk_size);
						//std::cout << "chunk kj: " << k << ", " << j << std::endl;
						//std::cout << "chunk: " << off.x << ", " << off.y << std::endl;
						b.get<Transform>()[n] = glm::translate(off);
						auto &r = b.get<OpaqueRender>()[n];
						r.pipeline = pipeline;
						r.material = material;
						r.model = model;
					}
				}

			pos = npos;
			pos_end = npos_end;
			scale = nscale;
		}
	}

public:
	void run(void)
	{
		auto pipeline_pool = PipelinePool(64);
		auto material_pool = MaterialPool(64);
		auto model_pool = ModelPool(64);
		size_t image_count = 1;
		auto image_pool = Pool<Vk::ImageAllocation>(image_count);
		auto image_view_pool = Pool<Vk::ImageView>(image_count);

		//auto model_world = model_pool.allocate();
		//*model_world = world.createChunk(m_r, ivec2(0));

		auto pipeline_opaque = pipeline_pool.allocate();
		*pipeline_opaque = m_r.createPipeline3D("sha/opaque", sizeof(int32_t));
		pipeline_opaque->pushDynamic<MVP>();
		pipeline_opaque->pushDynamic<MV_normal>();
		pipeline_opaque->pushDynamic<MW_local>();

		auto sampler_norm_n = m_r.device.createSampler(VkSamplerCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.anisotropyEnable = true,
			.maxAnisotropy = 16.0f,
			.maxLod = VK_LOD_CLAMP_NONE
		});

		auto material_albedo = material_pool.allocate();
		*reinterpret_cast<int32_t*>(material_albedo) = 0;

		{
			auto img0 = image_pool.allocate();
			*img0 = m_r.loadImage("res/img/grass.png", true);
			auto view0 = image_view_pool.allocate();
			*view0 = m_r.createImageView(*img0, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

			VkDescriptorImageInfo image_infos[] {
				{sampler_norm_n, *view0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
			};
			m_r.bindCombinedImageSamplers(0, array_size(image_infos), image_infos);
		}

		/*{
			auto [b, n] = m_m.addBrush<Id, Transform, MVP, MV_normal, MW_local, OpaqueRender>(1);
			b.get<Transform>()[n] = glm::scale(glm::dvec3(1.0));
			auto &r = b.get<OpaqueRender>()[n];
			r.pipeline = pipeline_opaque;
			r.material = material_albedo;
			r.model = model_world;
		}*/
		gen_chunks(pipeline_opaque, material_albedo);

		auto bef = std::chrono::high_resolution_clock::now();
		double t = 0.0;
		auto camera_pos = glm::dvec3(0.0);
		bool cursor_mode = false;
		m_r.setCursorMode(false);
		auto base_cursor = glm::dvec2(0.0);
		bool esc_prev = false;
		bool esc = false;
		bool first_it = true;
		while (true) {
			m_r.resetFrame();
			auto now = std::chrono::high_resolution_clock::now();
			auto delta = static_cast<std::chrono::duration<double>>(now - bef).count();
			bef = now;
			t += delta;
			glm::dmat4 last_view, view, proj;
			const double near = 0.1, far = 1000.0;
			const double ang_rad = pi / 180.0;

			m_r.pollEvents();
			if (m_r.shouldClose())
				break;

			const double ratio = static_cast<double>(m_r.swapchainExtent().width) / static_cast<double>(m_r.swapchainExtent().height),
				fov = 70.0 * ang_rad;
			{
				proj = glm::perspectiveLH_ZO<float>(fov, ratio, far, near);
				proj[1][1] *= -1.0;

				auto cursor = base_cursor;
				if (!cursor_mode)
					cursor = m_r.cursor() - base_cursor;
				{
					esc_prev = esc;
					esc = m_r.keyState(GLFW_KEY_ESCAPE);
				}
				if (esc && !esc_prev) {
					if (!cursor_mode)
						base_cursor = cursor;
					else
						base_cursor = m_r.cursor() - base_cursor;
					cursor_mode = !cursor_mode;
					m_r.setCursorMode(cursor_mode);
				}
				const float sensi = 0.1;
				double move = m_r.keyState(GLFW_KEY_LEFT_SHIFT) ? 20.0 : 2.0;
				auto view_rot = glm::rotate((-cursor.x * ang_rad) * sensi, glm::dvec3(0.0, 1.0, 0.0));
				view_rot = glm::rotate(std::clamp(-cursor.y * sensi, -90.0, 90.0) * ang_rad, glm::dvec3(1.0, 0.0, 0.0)) * view_rot;
				auto view_rot_inverse = glm::inverse(view_rot);
				auto dir_fwd_w = view_rot_inverse * glm::dvec4(0.0, 0.0, 1.0, 0.0);
				auto dir_fwd = glm::dvec3(dir_fwd_w.x, dir_fwd_w.y, dir_fwd_w.z);
				auto dir_side_w = view_rot_inverse * glm::dvec4(1.0, 0.0, 0.0, 0.0);
				auto dir_side = glm::dvec3(dir_side_w.x, dir_side_w.y, dir_side_w.z);
				if (m_r.keyState(GLFW_KEY_W))
					camera_pos += dir_fwd * move * delta;
				if (m_r.keyState(GLFW_KEY_S))
					camera_pos -= dir_fwd * move * delta;
				if (m_r.keyState(GLFW_KEY_A))
					camera_pos -= dir_side * move * delta;
				if (m_r.keyState(GLFW_KEY_D))
					camera_pos += dir_side * move * delta;
				last_view = view;
				view = view_rot * glm::translate(-camera_pos);
				if (first_it)
					last_view = view;
				auto vp = proj * view;

				m_m.query<MVP>([&](Brush &b){
					auto size = b.size();
					auto mvp = b.get<MVP>();
					auto mv_normal = b.get<MV_normal>();
					auto mw_local = b.get<MW_local>();
					auto trans = b.get<Transform>();
					for (size_t i = 0; i < size; i++) {
						mvp[i] = vp * trans[i];
						auto normal = view * trans[i];
						for (size_t j = 0; j < 3; j++)
							for (size_t k = 0; k < 3; k++) {
								mv_normal[i][j][k] = normal[j][k];
								mw_local[i][j][k] = trans[i][j][k];
							}
					}
				});
			}
			m_r.render(m_m, Camera{last_view, view, proj, static_cast<float>(far), static_cast<float>(near), glm::vec2(ratio, -1.0) * glm::vec2(std::tan(fov / 2.0))});
			first_it = false;
		}

		m_r.waitIdle();
		m_r.device.destroy(sampler_norm_n);
		pipeline_pool.destroy(m_r.device);
		model_pool.destroy(m_r.allocator);
		image_view_pool.destroyUsing(m_r.device);
		image_pool.destroyUsing(m_r.allocator);
	}

	Game(bool validate, bool useRenderDoc) :
		m_r(3, validate, useRenderDoc)
	{
	}
	~Game(void)
	{
	}
};

int main(int argc, char **argv)
{
	bool validate = false;
	bool is_render_doc = false;
	for (int i = 1; i < argc; i++) {
		if (std::strcmp(argv[i], "-v") == 0)
			validate = true;
		if (std::strcmp(argv[i], "-r") == 0)
			is_render_doc = true;
	}

	auto g = Game(validate || is_debug, is_render_doc);
	g.run();
	return 0;
}