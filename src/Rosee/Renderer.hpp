#pragma once

#include <mutex>
#include <condition_variable>
#include <random>
#include "vector.hpp"
#include "Vk.hpp"
#include "Map.hpp"
#include "math.hpp"
#include "Pipeline.hpp"
#include "Material.hpp"
#include "Model.hpp"
#include "Pool.hpp"
#include <GLFW/glfw3.h>

namespace Rosee {

using PipelinePool = Pool<Pipeline>;
using MaterialPool = Pool<Material>;
using ModelPool = Pool<Model>;

struct Camera {
	glm::mat4 last_view;
	glm::mat4 view;
	glm::mat4 proj;
	float near;
	float far;
	glm::vec2 ratio;
};

namespace Vertex {

struct p2
{
	glm::vec2 p;
};

struct pnu
{
	glm::vec3 p;
	glm::vec3 n;
	glm::vec2 u;
};

}

class Renderer
{
	uint32_t m_frame_count;
	bool m_validate;
	bool m_use_render_doc;

	void throwGlfwError(void);
	glm::ivec2 m_window_last_pos;
	glm::ivec2 m_window_last_size;
	bool m_fullscreen = false;

	GLFWwindow *m_window;
	GLFWwindow *createWindow(void);
	svec2 getWindowSize(void) const;

	Vk::Instance m_instance;
	Vk::Instance createInstance(void);
	Vk::DebugUtilsMessengerEXT m_debug_messenger;
	Vk::DebugUtilsMessengerEXT createDebugMessenger(void);
	Vk::SurfaceKHR m_surface;
	Vk::SurfaceKHR createSurface(void);
	VkPhysicalDeviceProperties m_properties;
	VkPhysicalDeviceLimits m_limits;
	VkPhysicalDeviceFeatures m_features;
	VkPresentModeKHR m_present_mode;
	VkSurfaceFormatKHR m_surface_format;
	VkSurfaceCapabilitiesKHR m_surface_capabilities;
	uint32_t m_queue_family_graphics = ~0U;
	VkPhysicalDevice m_physical_device;

public:
	Vk::Device device;

private:
	Vk::Device createDevice(void);

public:
	const VkFormat format_depth;

private:
	VkFormat getFormatDepth(void);
	const VkSampleCountFlags m_supported_sample_counts;
	VkSampleCountFlags getSupportedSampleCounts(void);
	VkSampleCountFlagBits fitSampleCount(VkSampleCountFlagBits sampleCount) const;

private:
	Vk::PipelineCache m_pipeline_cache;

public:
	Vk::Allocator allocator;

private:
	Vk::Allocator createAllocator(void);
	Vk::Queue m_queue;

public:
	void waitIdle(void)
	{
		m_queue.waitIdle();
	}

private:
	VkExtent2D m_swapchain_extent;
	uint32_t m_swapchain_mip_levels;
	VkExtent2D m_swapchain_extent_mip;
	vector<VkExtent2D> m_swapchain_extent_mips;
	static uint32_t extentLog2(uint32_t val);
	static uint32_t extentMipLevels(const VkExtent2D &extent);
	static uint32_t nextExtentMip(uint32_t extent);

public:
	const VkExtent2D& swapchainExtent(void) const { return m_swapchain_extent; }

private:
	struct PipelineViewportState {
		VkViewport viewport;
		VkRect2D scissor;
		VkPipelineViewportStateCreateInfo ci;
	} m_pipeline_viewport_state;
	Vk::SwapchainKHR m_swapchain;
	Vk::SwapchainKHR createSwapchain(void);
	vector<VkImage> m_swapchain_images;
	vector<Vk::ImageView> m_swapchain_image_views;

public:
	Vk::ImageView createImageView(VkImage image, VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspect);
	Vk::ImageView createImageViewMip(VkImage image, VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspect,
		uint32_t baseMipLevel, uint32_t levelCount);

private:
	vector<Vk::ImageView> createSwapchainImageViews(void);

	Vk::CommandPool m_command_pool;
	Vk::CommandPool m_transfer_command_pool;
	Vk::CommandBuffer m_transfer_cmd;

	static inline constexpr uint32_t s0_sampler_count = 1;
	static inline constexpr uint32_t sets_per_frame = 5;
	static inline constexpr uint32_t sets_per_frame_mip = 1;

	Vk::DescriptorSetLayout m_descriptor_set_layout_0;
	Vk::DescriptorSetLayout createDescriptorSetLayout0(void);
	Vk::DescriptorSetLayout m_descriptor_set_layout_dynamic;
	Vk::DescriptorSetLayout createDescriptorSetLayoutDynamic(void);
	Vk::PipelineLayout m_pipeline_layout_descriptor_set;
	Vk::PipelineLayout createPipelineLayoutDescriptorSet(void);
	Vk::DescriptorPool m_descriptor_pool;
	Vk::DescriptorPool createDescriptorPool(void);
	Vk::DescriptorPool m_descriptor_pool_mip;
	Vk::DescriptorPool createDescriptorPoolMip(void);

	Vk::ShaderModule m_fwd_p2_module;
	Vk::BufferAllocation m_screen_vertex_buffer;
	Vk::BufferAllocation createScreenVertexBuffer(void);

	VkSampleCountFlagBits m_sample_count;

	Vk::RenderPass m_opaque_pass;
	Vk::RenderPass createOpaquePass(void);
	Vk::RenderPass m_depth_resolve_pass;
	Vk::RenderPass createDepthResolvePass(void);
	Vk::DescriptorSetLayout m_depth_resolve_set_layout;
	Vk::DescriptorSetLayout createDepthResolveSetLayout(void);
	Pipeline m_depth_resolve_pipeline;
	Pipeline createDepthResolvePipeline(void);
	Pipeline m_depth_acc_pipeline;
	Pipeline createDepthAccPipeline(void);
	Vk::RenderPass m_illumination_pass;
	Vk::RenderPass createIlluminationPass(void);
	Vk::DescriptorSetLayout m_illumination_set_layout;
	Vk::DescriptorSetLayout createIlluminationSetLayout(void);
	Pipeline m_illumination_pipeline;
	Pipeline createIlluminationPipeline(void);
	Vk::RenderPass m_wsi_pass;
	Vk::RenderPass createWsiPass(void);
	Vk::DescriptorSetLayout m_wsi_set_layout;
	Vk::DescriptorSetLayout createWsiSetLayout(void);
	Pipeline m_wsi_pipeline;
	Pipeline createWsiPipeline(void);

	Vk::Sampler m_sampler_fb;
	Vk::Sampler createSamplerFb(void);
	Vk::Sampler m_sampler_fb_mip;
	Vk::Sampler createSamplerFbMip(void);
	Vk::Sampler m_sampler_fb_lin;
	Vk::Sampler createSamplerFbLin(void);

	class Frame
	{
		Renderer &m_r;
		size_t m_i;
		Vk::CommandBuffer m_transfer_cmd;
		Vk::CommandBuffer m_cmd;
		Vk::Fence m_frame_done;
		Vk::Semaphore m_render_done;
		Vk::Semaphore m_image_ready;
		bool m_ever_submitted = false;

		VkDescriptorSet m_descriptor_set_0;
		VkDescriptorSet m_descriptor_set_dynamic;

		static inline constexpr size_t dyn_buffer_size = 2000000;	// large for now, to eventually reduce to 2MB
		size_t m_dyn_buffer_size;
		void *m_dyn_buffer_staging_ptr;
		Vk::BufferAllocation m_dyn_buffer_staging;
		Vk::BufferAllocation createDynBufferStaging(void);
		Vk::BufferAllocation m_dyn_buffer;

		friend class Renderer;

		Vk::ImageView createFbImage(VkFormat format, VkImageAspectFlags aspect, VkImageUsageFlags usage, Vk::ImageAllocation *pAllocation);
		Vk::ImageView createFbImageMs(VkFormat format, VkImageAspectFlags aspect, VkImageUsageFlags usage, Vk::ImageAllocation *pAllocation);
		Vk::ImageView createFbImageMip(VkFormat format, VkImageAspectFlags aspect, VkImageUsageFlags usage, Vk::ImageAllocation *pAllocation);

		Vk::ImageAllocation m_depth_buffer;
		Vk::ImageView m_depth_buffer_view;
		Vk::ImageAllocation m_cdepth;
		Vk::ImageView m_cdepth_view;
		Vk::ImageAllocation m_depth;
		Vk::ImageView m_depth_view;
		Vk::ImageView m_depth_first_mip_view;
		Vk::ImageAllocation m_albedo;
		Vk::ImageView m_albedo_view;
		Vk::ImageAllocation m_normal;
		Vk::ImageView m_normal_view;

		Vk::ImageAllocation m_step;
		Vk::ImageView m_step_view;
		Vk::ImageAllocation m_acc;
		Vk::ImageView m_acc_view;
		Vk::ImageAllocation m_direct_light;
		Vk::ImageView m_direct_light_view;
		Vk::ImageAllocation m_path_pos;
		Vk::ImageView m_path_pos_view;
		Vk::ImageAllocation m_path_albedo;
		Vk::ImageView m_path_albedo_view;
		Vk::ImageAllocation m_path_direct_light;
		Vk::ImageView m_path_direct_light_view;
		Vk::ImageAllocation m_output;
		Vk::ImageView m_output_view;

		Vk::Framebuffer m_opaque_fb;
		Vk::Framebuffer createOpaqueFb(void);
		Vk::Framebuffer m_depth_resolve_fb;
		Vk::Framebuffer createDepthResolveFb(void);
		VkDescriptorSet m_depth_resolve_set;
		vector<Vk::ImageView> m_depth_acc_views;
		vector<Vk::ImageView> createDepthAccViews(void);
		vector<Vk::Framebuffer> m_depth_acc_fbs;
		vector<Vk::Framebuffer> createDepthAccFbs(void);
		vector<VkDescriptorSet> m_depth_acc_sets;
		vector<VkDescriptorSet> createDepthAccSets(const VkDescriptorSet *pDescriptorSetsMip);
		Vk::Framebuffer m_illumination_fb;
		Vk::Framebuffer createIlluminationFb(void);
		VkDescriptorSet m_illumination_set;
		Vk::BufferAllocation m_illumination_buffer;
		Vk::BufferAllocation createIlluminationBuffer(void);
		Vk::Framebuffer m_wsi_fb;
		Vk::Framebuffer createWsiFb(void);
		VkDescriptorSet m_wsi_set;

		struct Illumination {
			glm::mat4 cam_proj;
			glm::mat4 view;
			glm::mat4 view_normal;
			glm::mat4 view_inv;
			glm::mat4 view_normal_inv;
			glm::mat4 last_view;
			glm::mat4 last_view_inv;
			glm::mat4 view_cur_to_last;
			glm::mat4 view_last_to_cur;
			glm::mat4 view_last_to_cur_normal;
			glm::vec4 rnd_sun[256];
			glm::vec4 rnd_diffuse[256];
			glm::vec3 sun;
			float _pad;
			glm::vec2 size;
			glm::vec2 size_inv;
			glm::vec2 depth_size;
			glm::vec2 ratio;
			float cam_near;
			float cam_far;
			float cam_a;
			float cam_b;
		};

	public:
		Frame(Renderer &r, size_t i, VkCommandBuffer transferCmd, VkCommandBuffer cmd,
			VkDescriptorSet descriptorSet0, VkDescriptorSet descriptorSetDynamic,
			VkDescriptorSet descriptorSetDepthResolve,
			VkDescriptorSet descriptorSetIllum, VkDescriptorSet descriptorSetWsi,
			const VkDescriptorSet *pDescriptorSetsMip,
			Vk::BufferAllocation dynBuffer);

		void reset(void);
		void render(Map &map, const Camera &camera);
		void destroy(bool with_ext_res = false);

	private:
		void render_subset(Map &map, cmp_id render_id);
	};

	vector<Frame> m_frames;
	vector<Frame> createFrames(void);
	size_t m_current_frame = 0;

	void bindFrameDescriptors(void);
	void recreateSwapchain(void);

	Vk::ShaderModule loadShaderModule(VkShaderStageFlagBits stage, const char *path) const;
	static VkPipelineShaderStageCreateInfo initPipelineStage(VkShaderStageFlagBits stage, VkShaderModule module);

public:
	//Pipeline createPipeline(const char *stagesPath, uint32_t pushConstantRange);
	Pipeline createPipeline3D(const char *stagesPath, uint32_t pushConstantRange);

private:
	Pool<Pipeline> m_pipeline_pool;
	Pool<Model> m_model_pool;

public:
	Vk::BufferAllocation createVertexBuffer(size_t size);
	Model loadModel(const char *path);
	Vk::ImageAllocation loadImage(const char *path, bool gen_mips);

	void bindCombinedImageSamplers(uint32_t firstSampler, uint32_t imageInfoCount, const VkDescriptorImageInfo *pImageInfos);

private:
	bool m_keys_prev[GLFW_KEY_LAST];
	bool m_keys[GLFW_KEY_LAST];
	glm::dvec2 m_cursor = glm::dvec2(0.0, 0.0);
	static inline constexpr size_t key_update_count = 7;
	static size_t m_keys_update[key_update_count];
	size_t m_next_input = 0;
	size_t m_pending_cursor_mode = ~0ULL;
	std::mutex m_next_input_mutex;
	std::condition_variable m_next_input_cv;
	std::mutex m_input_mutex;
	std::mutex m_render_mutex;

	std::mt19937_64 m_rnd;

public:
	Renderer(uint32_t frameCount, bool validate, bool useRenderDoc);
	~Renderer(void);

	void pollEvents(void);
	bool shouldClose(void) const;
	bool keyState(int glfw_key);
	bool keyPressed(int glfw_key);
	bool keyReleased(int glfw_key);
	glm::dvec2 cursor(void);

	void setCursorMode(bool show);

	void resetFrame(void);
	void render(Map &map, const Camera &camera);

	double zrand(void);
};

}