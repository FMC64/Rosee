#pragma once

#include "vector.hpp"
#include "Vk.hpp"
#include "Map.hpp"
#include <GLFW/glfw3.h>
#include "math.hpp"
#include <mutex>
#include <condition_variable>

namespace Rosee {

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
	Vk::Device m_device;
	Vk::Device createDevice(void);
	Vk::PipelineCache m_pipeline_cache;
	Vk::Allocator m_allocator;
	Vk::Allocator createAllocator(void);
	Vk::Queue m_queue;
	VkExtent2D m_swapchain_extent;
	struct PipelineViewportState {
		VkViewport viewport;
		VkRect2D scissor;
		VkPipelineViewportStateCreateInfo ci;
	} m_pipeline_viewport_state;
	Vk::SwapchainKHR m_swapchain;
	Vk::SwapchainKHR createSwapchain(void);
	vector<VkImage> m_swapchain_images;
	vector<Vk::ImageView> m_swapchain_image_views;
	vector<Vk::ImageView> createSwapchainImageViews(void);

	Vk::RenderPass m_opaque_pass;
	Vk::RenderPass createOpaquePass(void);
	vector<Vk::Framebuffer> m_opaque_fbs;
	vector<Vk::Framebuffer> createOpaqueFbs(void);
	Vk::CommandPool m_command_pool;

	Vk::DescriptorSetLayout m_descriptor_set_layout_dynamic;
	Vk::DescriptorSetLayout createDescriptorSetLayoutDynamic(void);
	Vk::DescriptorPool m_descriptor_pool_dynamic;
	Vk::DescriptorPool createDescriptorPoolDynamic(void);

	class Frame
	{
		Renderer &m_r;
		Vk::CommandBuffer m_transfer_cmd;
		Vk::CommandBuffer m_cmd;
		Vk::Fence m_frame_done;
		Vk::Semaphore m_render_done;
		Vk::Semaphore m_image_ready;
		bool m_ever_submitted = false;

		VkDescriptorSet m_descriptor_set_dynamic;

		static inline constexpr size_t dyn_buffer_size = 64000000;	// large for now, to eventually reduce to 2MB
		size_t m_dyn_buffer_size;
		void *m_dyn_buffer_staging_ptr;
		Vk::BufferAllocation m_dyn_buffer_staging;
		Vk::BufferAllocation createDynBufferStaging(void);
		Vk::BufferAllocation m_dyn_buffer;
		Vk::BufferAllocation createDynBuffer(void);

	public:
		Frame(Renderer &r, VkCommandBuffer transferCmd, VkCommandBuffer cmd, VkDescriptorSet descriptorSetDynamic);
		~Frame(void);

		void reset(void);
		void render(Map &map);
	};

	vector<Frame> m_frames;
	vector<Frame> createFrames(void);
	size_t m_current_frame = 0;

	void recreateSwapchain(void);

	Vk::ShaderModule loadShaderModule(const char *path) const;
	static VkPipelineShaderStageCreateInfo initPipelineStage(VkShaderStageFlagBits stage, VkShaderModule module);

	Vk::PipelineLayout m_pipeline_layout_empty;
	Vk::PipelineLayout createPipelineLayoutEmpty(void);

	Vk::ShaderModule m_particle_vert;
	Vk::ShaderModule m_particle_frag;
	Vk::Pipeline m_particle_pipeline;
	Vk::Pipeline createParticlePipeline(void);

	Vk::BufferAllocation m_point_buffer;
	Vk::BufferAllocation createPointBuffer(void);

	bool m_keys_prev[GLFW_KEY_LAST];
	bool m_keys[GLFW_KEY_LAST];
	static inline constexpr size_t key_update_count = 1;
	static size_t m_keys_update[key_update_count];
	size_t m_next_input = 0;
	std::mutex m_next_input_mutex;
	std::condition_variable m_next_input_cv;
	std::mutex m_input_mutex;
	std::mutex m_render_mutex;

public:
	Renderer(uint32_t frameCount, bool validate, bool useRenderDoc);
	~Renderer(void);

	void pollEvents(void);
	bool shouldClose(void) const;
	bool keyState(int glfw_key);
	bool keyPressed(int glfw_key);
	bool keyReleased(int glfw_key);

	void resetFrame(void);
	void render(Map &map);
};

}