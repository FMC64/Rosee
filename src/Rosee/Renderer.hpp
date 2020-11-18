#pragma once

#include "vector.hpp"
#include "Vk.hpp"
#include <GLFW/glfw3.h>
#include "math.hpp"

namespace Rosee {

class Renderer
{
	size_t m_frame_count;
	bool m_validate;
	bool m_use_render_doc;

	void throwGlfwError(void);
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
	Vk::Device m_device;
	Vk::Device createDevice(void);
	Vk::Queue m_queue;
	Vk::SwapchainKHR m_swapchain;
	Vk::SwapchainKHR createSwapchain(void);

	Vk::RenderPass m_opaque_pass;
	Vk::RenderPass createOpaquePass(void);
	Vk::CommandPool m_command_pool;

	class Frame
	{
		VkCommandBuffer m_cmd;

	public:
		Frame(Renderer &r, VkCommandBuffer cmd);
		~Frame(void);
	};

	vector<Frame> m_frames;
	vector<Frame> createFrames(void);

public:
	Renderer(size_t frameCount, bool validate, bool useRenderDoc);
	~Renderer(void);

	void pollEvents(void);
	bool shouldClose(void) const;
};

}