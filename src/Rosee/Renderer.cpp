#include "Renderer.hpp"
#include "c.hpp"
#include <map>
#include <iostream>

namespace Rosee {

void Renderer::throwGlfwError(void)
{
	const char *msg;
	glfwGetError(&msg);
	throw std::runtime_error(msg);
}

GLFWwindow* Renderer::createWindow(void)
{
	if (glfwInit() != GLFW_TRUE)
		throwGlfwError();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto res = glfwCreateWindow(1600, 900, "Rosee", nullptr, nullptr);
	if (res == nullptr)
		throwGlfwError();
	return res;
}

svec2 Renderer::getWindowSize(void) const
{
	int w, h;
	glfwGetWindowSize(m_window, &w, &h);
	return svec2(w, h);
}

Vk::Instance Renderer::createInstance(void)
{
	VkInstanceCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

	VkApplicationInfo ai{};
	ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ai.pApplicationName = "SUNREN";
	ai.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	ai.pEngineName = "Rosee";
	ai.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	ai.apiVersion = VK_API_VERSION_1_0;
	ci.pApplicationInfo = &ai;

	uint32_t glfw_ext_count;
	auto glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
	if (glfw_exts == nullptr)
		throwGlfwError();

	size_t layer_count = 0;
	const char *layers[16];
	size_t ext_count = 0;
	const char *exts[glfw_ext_count + 16];

	for (uint32_t i = 0; i < glfw_ext_count; i++)
		exts[ext_count++] = glfw_exts[i];
	if (m_validate) {
		layers[layer_count++] = "VK_LAYER_KHRONOS_validation";
		exts[ext_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	}
	if (m_use_render_doc)
		layers[layer_count++] = "VK_LAYER_RENDERDOC_Capture";

	ci.enabledLayerCount = layer_count;
	ci.ppEnabledLayerNames = layers;
	ci.enabledExtensionCount = ext_count;
	ci.ppEnabledExtensionNames = exts;

	std::cout << "VkInstance layers:" << std::endl;
	for (size_t i = 0; i < layer_count; i++)
		std::cout << layers[i] << std::endl;
	std::cout << std::endl;
	std::cout << "VkInstance extensions:" << std::endl;
	for (size_t i = 0; i < ext_count; i++)
		std::cout << exts[i] << std::endl;
	std::cout << std::endl;

	return Vk::createInstance(ci);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_cb(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void*)
{
	static const std::map<VkDebugUtilsMessageSeverityFlagBitsEXT, const char*> sever_table {
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT, "VERBOSE"},
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT, "INFO"},
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, "WARNING"},
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, "ERROR"}
	};

	static const std::map<VkDebugUtilsMessageTypeFlagBitsEXT, const char*> type_table {
		{VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, "General"},
		{VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT, "Validation"},
		{VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, "Performance"}
	};

	if (messageSeverity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT &&
	messageSeverity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		auto comma = "";
		std::cerr << "[";
		for (auto &p : sever_table)
			if (p.first & messageSeverity) {
				std::cerr << p.second;
				comma = ", ";
			}
		std::cerr << "] -> {";
		comma = "";
		for (auto &p : type_table)
			if (p.first & messageType) {
				std::cerr << p.second;
				comma = ", ";
			}
		std::cerr << "}" << std::endl;

		std::cerr << pCallbackData->pMessage << std::endl;
	}

	return VK_FALSE;
}

Vk::DebugUtilsMessengerEXT Renderer::createDebugMessenger(void)
{
	if (!m_validate)
		return VK_NULL_HANDLE;

	VkDebugUtilsMessengerCreateInfoEXT ci{};
	ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	ci.pfnUserCallback = debug_messenger_cb;

	VkDebugUtilsMessengerEXT res;
	vkAssert(m_instance.getProcAddr<PFN_vkCreateDebugUtilsMessengerEXT>("vkCreateDebugUtilsMessengerEXT")(m_instance, &ci, nullptr, &res));
	return res;
}

Vk::SurfaceKHR Renderer::createSurface(void)
{
	VkSurfaceKHR res;
	vkAssert(glfwCreateWindowSurface(m_instance, m_window, nullptr, &res));
	return res;
}

Vk::Device Renderer::createDevice(void)
{
	uint32_t physical_device_count;
	vkAssert(vkEnumeratePhysicalDevices(m_instance, &physical_device_count, nullptr));
	VkPhysicalDevice physical_devices[physical_device_count];
	VkPhysicalDeviceProperties physical_devices_properties[physical_device_count];
	VkPhysicalDeviceFeatures physical_devices_features[physical_device_count];
	uint32_t physical_devices_gqueue_families[physical_device_count];
	vkAssert(vkEnumeratePhysicalDevices(m_instance, &physical_device_count, physical_devices));

	for (uint32_t i = 0; i < physical_device_count; i++) {
		vkGetPhysicalDeviceProperties(physical_devices[i], &physical_devices_properties[i]);
		vkGetPhysicalDeviceFeatures(physical_devices[i], &physical_devices_features[i]);
	}

	static auto required_features = VkPhysicalDeviceFeatures {
	};
	static const char *required_exts[] {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	uint32_t chosen = ~0U;
	size_t chosen_score = 0;

	for (uint32_t i = 0; i < physical_device_count; i++) {
		auto &dev = physical_devices[i];
		auto &properties = physical_devices_properties[i];
		auto &features = physical_devices_features[i];

		constexpr size_t f_b_count = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
		auto req_f_set = reinterpret_cast<VkBool32*>(&required_features);
		auto got_f_set = reinterpret_cast<VkBool32*>(&features);
		bool has_all_req_features = true;
		for (size_t j = 0; j < f_b_count; j++)
			if (req_f_set[j] && !got_f_set[j])
				has_all_req_features = false;
		if (!has_all_req_features)
			continue;
		uint32_t ext_count;
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &ext_count, nullptr);
		VkExtensionProperties exts[ext_count];
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &ext_count, exts);
		bool ext_missing = false;
		for (size_t j = 0; j < array_size(required_exts); j++) {
			bool found = false;
			for (size_t k = 0; k < ext_count; k++)
				if (std::strcmp(required_exts[j], exts[k].extensionName) == 0) {
					found = true;
					break;
				}
			if (!found) {
				ext_missing = true;
				break;
			}
		}
		if (ext_missing)
			continue;

		uint32_t queue_family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, nullptr);
		VkQueueFamilyProperties queue_families[queue_family_count];
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, queue_families);

		physical_devices_gqueue_families[i] = ~0U;
		size_t gbits = ~0ULL;
		for (uint32_t j = 0; j < queue_family_count; j++) {
			auto &cur = queue_families[j];
			size_t fbits = 0;
			for (size_t k = 0; k < sizeof(cur.queueFlags) * 8; k++)
				if (cur.queueFlags & (1 << k))
					fbits++;
			VkBool32 present_supported;
			vkAssert(vkGetPhysicalDeviceSurfaceSupportKHR(dev, j, m_surface, &present_supported));
			if (present_supported && (cur.queueFlags & VK_QUEUE_GRAPHICS_BIT) && fbits < gbits) {
				physical_devices_gqueue_families[i] = j;
				gbits = fbits;
			}
		}
		if (physical_devices_gqueue_families[i] == ~0U)
			continue;

		uint32_t present_mode_count;
		vkAssert(vkGetPhysicalDeviceSurfacePresentModesKHR(dev, m_surface, &present_mode_count, nullptr));
		if (present_mode_count == 0)
			continue;
		uint32_t surface_format_count;
		vkAssert(vkGetPhysicalDeviceSurfaceFormatsKHR(dev, m_surface, &surface_format_count, nullptr));
		if (surface_format_count == 0)
			continue;

		size_t score = 1;
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score++;
		if (score > chosen_score) {
			chosen = i;
			chosen_score = score;
		}
	}

	if (chosen == ~0U)
		throw std::runtime_error("No compatible GPU found on your system.");
	m_properties = physical_devices_properties[chosen];
	m_limits = m_properties.limits;
	m_features = physical_devices_features[chosen];

	{
		uint32_t present_mode_count;
		vkAssert(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[chosen], m_surface, &present_mode_count, nullptr));
		VkPresentModeKHR present_modes[present_mode_count];
		vkAssert(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[chosen], m_surface, &present_mode_count, present_modes));
		m_present_mode = present_modes[0];
		for (size_t i = 0; i < present_mode_count; i++)
			if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
				m_present_mode = present_modes[i];
				break;
			}

		uint32_t surface_format_count;
		vkAssert(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_devices[chosen], m_surface, &surface_format_count, nullptr));
		VkSurfaceFormatKHR surface_formats[surface_format_count];
		vkAssert(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_devices[chosen], m_surface, &surface_format_count, surface_formats));
		m_surface_format = surface_formats[0];
		for (size_t i = 0; i < surface_format_count; i++)
			if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) {
				m_surface_format = surface_formats[i];
				break;
			}

		vkAssert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_devices[chosen], m_surface, &m_surface_capabilities));
	}

	m_queue_family_graphics = physical_devices_gqueue_families[chosen];

	std::cout << "Vulkan device: " << m_properties.deviceName << std::endl;
	std::cout << std::endl;

	VkDeviceCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	VkDeviceQueueCreateInfo qci{};
	qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	qci.queueFamilyIndex = m_queue_family_graphics;
	qci.queueCount = 1;

	const float prio[] {
		1.0f
	};
	qci.pQueuePriorities = prio;

	VkDeviceQueueCreateInfo qcis[] {qci};
	ci.queueCreateInfoCount = array_size(qcis);
	ci.pQueueCreateInfos = qcis;
	ci.pEnabledFeatures = &required_features;
	ci.enabledExtensionCount = array_size(required_exts);
	ci.ppEnabledExtensionNames = required_exts;

	return Vk::createDevice(physical_devices[chosen], ci);
}

Vk::SwapchainKHR Renderer::createSwapchain(void)
{
	VkSwapchainCreateInfoKHR ci{};
	ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	ci.surface = m_surface;
	ci.minImageCount = clamp(static_cast<uint32_t>(m_frame_count), m_surface_capabilities.minImageCount, m_surface_capabilities.maxImageCount);
	ci.imageFormat = m_surface_format.format;
	ci.imageColorSpace = m_surface_format.colorSpace;
	auto wins = getWindowSize();
	ci.imageExtent = VkExtent2D{clamp(static_cast<uint32_t>(wins.x), max(m_surface_capabilities.minImageExtent.width, static_cast<uint32_t>(1)), m_surface_capabilities.maxImageExtent.width),
		clamp(static_cast<uint32_t>(wins.y), max(m_surface_capabilities.minImageExtent.height, static_cast<uint32_t>(1)), m_surface_capabilities.maxImageExtent.height)};
	ci.imageArrayLayers = 1;
	ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ci.queueFamilyIndexCount = 1;
	ci.pQueueFamilyIndices = &m_queue_family_graphics;
	ci.preTransform = m_surface_capabilities.currentTransform;
	ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	ci.presentMode = m_present_mode;
	ci.clipped = VK_TRUE;

	return m_device.createSwapchainKHR(ci);
}

vector<Vk::ImageView> Renderer::createSwapchainImageViews(void)
{
	vector<Vk::ImageView> res;
	res.reserve(m_swapchain_images.size());
	for (auto &i : m_swapchain_images) {
		VkImageViewCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.image = i;
		ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ci.format = m_surface_format.format;
		ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ci.subresourceRange.baseMipLevel = 0;
		ci.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		ci.subresourceRange.baseArrayLayer = 0;
		ci.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		res.emplace(m_device.createImageView(ci));
	}
	return res;
}

Vk::RenderPass Renderer::createOpaquePass(void)
{
	VkRenderPassCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	VkAttachmentDescription atts[] {
		{0, VK_FORMAT_B8G8R8A8_SRGB, Vk::SampleCount1Bit, Vk::AttachmentLoadOp_DontCare, Vk::AttachmentStoreOp_Store,	// wsi 0
			Vk::AttachmentLoadOp_DontCare, Vk::AttachmentStoreOp_DontCare,
			Vk::ImageLayout_Undefined, Vk::ImageLayout_PresentSrcKhr}
	};
	VkAttachmentReference wsi {0, Vk::ImageLayout_ColorAttachmentOptimal};
	VkSubpassDescription subpasses[] {
		{0, VK_PIPELINE_BIND_POINT_GRAPHICS,
			0, nullptr,		// input
			1, &wsi, nullptr,	// color, resolve
			nullptr,		// depth
			0, nullptr}		// preserve
	};

	ci.attachmentCount = array_size(atts);
	ci.pAttachments = atts;
	ci.subpassCount = array_size(subpasses);
	ci.pSubpasses = subpasses;

	return m_device.createRenderPass(ci);
}

vector<Vk::Framebuffer> Renderer::createOpaqueFbs(void)
{
	vector<Vk::Framebuffer> res;
	res.reserve(m_swapchain_image_views.size());

	auto wins = getWindowSize();

	for (auto &i : m_swapchain_image_views) {
		VkFramebufferCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		ci.renderPass = m_opaque_pass;
		ci.attachmentCount = 1;
		ci.pAttachments = i.ptr();
		ci.width = wins.x;
		ci.height = wins.y;
		ci.layers = 1;
		res.emplace(m_device.createFramebuffer(ci));
	}
	return res;
}

vector<Renderer::Frame> Renderer::createFrames(void)
{
	VkCommandBuffer cmds[m_frame_count];
	m_device.allocateCommandBuffers(m_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_frame_count, cmds);

	vector<Renderer::Frame> res;
	res.reserve(m_frame_count);
	for (size_t i = 0; i < m_frame_count; i++)
		res.emplace(*this, cmds[i]);
	return res;
}

Renderer::Renderer(size_t frameCount, bool validate, bool useRenderDoc) :
	m_frame_count(frameCount),
	m_validate(validate),
	m_use_render_doc(useRenderDoc),
	m_window(createWindow()),
	m_instance(createInstance()),
	m_debug_messenger(createDebugMessenger()),
	m_surface(createSurface()),
	m_device(createDevice()),
	m_queue(m_device.getQueue(m_queue_family_graphics, 0)),
	m_swapchain(createSwapchain()),
	m_swapchain_images(m_device.getSwapchainImages(m_swapchain)),
	m_swapchain_image_views(createSwapchainImageViews()),
	m_opaque_pass(createOpaquePass()),
	m_opaque_fbs(createOpaqueFbs()),
	m_command_pool(m_device.createCommandPool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		m_queue_family_graphics)),
	m_frames(createFrames())
{
}

Renderer::~Renderer(void)
{
	m_queue.waitIdle();

	m_frames.clear();
	m_device.destroy(m_command_pool);
	for (auto &f : m_opaque_fbs)
		m_device.destroy(f);
	m_device.destroy(m_opaque_pass);
	for (auto &v : m_swapchain_image_views)
		m_device.destroy(v);
	m_device.destroy(m_swapchain);
	m_device.destroy();
	m_instance.destroy(m_surface);
	if (m_debug_messenger)
		m_instance.getProcAddr<PFN_vkDestroyDebugUtilsMessengerEXT>("vkDestroyDebugUtilsMessengerEXT")(m_instance, m_debug_messenger, nullptr);
	m_instance.destroy();
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Renderer::pollEvents(void)
{
	glfwPollEvents();
}

bool Renderer::shouldClose(void) const
{
	return glfwWindowShouldClose(m_window);
}

void Renderer::render(void)
{
	m_frames[m_current_frame].render();
	m_current_frame = (m_current_frame + 1) % m_frame_count;
}

Renderer::Frame::Frame(Renderer &r, VkCommandBuffer cmd) :
	m_r(r),
	m_cmd(cmd),
	m_frame_done(r.m_device.createFence(0)),
	m_render_done(r.m_device.createSemaphore()),
	m_image_ready(r.m_device.createSemaphore())
{
}

Renderer::Frame::~Frame(void)
{
	m_r.m_device.destroy(m_frame_done);
	m_r.m_device.destroy(m_render_done);
	m_r.m_device.destroy(m_image_ready);
}

void Renderer::Frame::render(void)
{
	if (m_ever_submitted) {
		m_r.m_device.wait(m_frame_done);
		m_r.m_device.reset(m_frame_done);
	}

	auto wins = m_r.getWindowSize();

	uint32_t swapchain_index;
	vkAssert(vkAcquireNextImageKHR(m_r.m_device, m_r.m_swapchain, ~0ULL, m_image_ready, VK_NULL_HANDLE, &swapchain_index));

	m_cmd.beginPrimary(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	{
		VkRenderPassBeginInfo bi{};
		bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		bi.renderPass = m_r.m_opaque_pass;
		bi.framebuffer = m_r.m_opaque_fbs[swapchain_index];
		bi.renderArea = VkRect2D{{0, 0}, {static_cast<uint32_t>(wins.x), static_cast<uint32_t>(wins.y)}};
		m_cmd.beginRenderPass(bi, VK_SUBPASS_CONTENTS_INLINE);
	}
	m_cmd.endRenderPass();
	m_cmd.end();

	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo si[] {
		{VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr,
			1, m_image_ready.ptr(),
			&wait_stage,
			1, m_cmd.ptr(),
			1, m_render_done.ptr()}
	};
	m_r.m_queue.submit(array_size(si), si, m_frame_done);

	VkPresentInfoKHR pi{};
	pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	pi.waitSemaphoreCount = 1;
	pi.pWaitSemaphores = m_render_done.ptr();
	pi.swapchainCount = 1;
	pi.pSwapchains = m_r.m_swapchain.ptr();
	pi.pImageIndices = &swapchain_index;
	m_r.m_queue.present(pi);

	m_ever_submitted = true;
}

}