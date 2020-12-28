#include "Renderer.hpp"
#include "c.hpp"
#include <map>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <ctime>
#include "../../dep/tinyobjloader/tiny_obj_loader.h"
#include "../../dep/stb/stb_image.h"

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
	glfwGetWindowPos(res, &m_window_last_pos.x, &m_window_last_pos.y);
	glfwGetWindowSize(res, &m_window_last_size.x, &m_window_last_size.y);
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
	auto enum_instance = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
	if (enum_instance == nullptr)
		m_instance_version = VK_API_VERSION_1_0;
	else {
		uint32_t max_version;
		vkAssert(enum_instance(&max_version));
		if (max_version >= VK_API_VERSION_1_1)
			m_instance_version = VK_API_VERSION_1_1;
		else
			m_instance_version = VK_API_VERSION_1_0;
	}
	if (m_instance_version == VK_API_VERSION_1_0)
		std::cout << "Vulkan 1.0" << std::endl;
	else if (m_instance_version == VK_API_VERSION_1_1)
		std::cout << "Vulkan 1.1" << std::endl;
	else
		std::cout << "Vulkan v" << m_instance_version << std::endl;

	VkInstanceCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

	VkApplicationInfo ai{};
	ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ai.pApplicationName = "SUNREN";
	ai.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	ai.pEngineName = "Rosee";
	ai.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	ai.apiVersion = m_instance_version;
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

	auto res = Vk::createInstance(ci);
	auto &e = Vk::ext;
#define EXT(name) e.name = res.getProcAddr<PFN_ ## name>(#name)
	// VK_KHR_swapchain
	EXT(vkGetPhysicalDeviceSurfaceSupportKHR);
	EXT(vkGetPhysicalDeviceSurfacePresentModesKHR);
	EXT(vkGetPhysicalDeviceSurfaceFormatsKHR);
	EXT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
	EXT(vkDestroySurfaceKHR);
	EXT(vkCreateSwapchainKHR);
	EXT(vkDestroySwapchainKHR);
	EXT(vkGetSwapchainImagesKHR);
	EXT(vkQueuePresentKHR);
	EXT(vkAcquireNextImageKHR);

	// VK_KHR_acceleration_structure
#undef EXT
	return res;
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
		.samplerAnisotropy = true
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

		if (properties.apiVersion < m_instance_version)
			continue;

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
			vkAssert(Vk::ext.vkGetPhysicalDeviceSurfaceSupportKHR(dev, j, m_surface, &present_supported));
			if (present_supported && (cur.queueFlags & VK_QUEUE_GRAPHICS_BIT) && fbits < gbits) {
				physical_devices_gqueue_families[i] = j;
				gbits = fbits;
			}
		}
		if (physical_devices_gqueue_families[i] == ~0U)
			continue;

		uint32_t present_mode_count;
		vkAssert(Vk::ext.vkGetPhysicalDeviceSurfacePresentModesKHR(dev, m_surface, &present_mode_count, nullptr));
		if (present_mode_count == 0)
			continue;
		uint32_t surface_format_count;
		vkAssert(Vk::ext.vkGetPhysicalDeviceSurfaceFormatsKHR(dev, m_surface, &surface_format_count, nullptr));
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
	m_physical_device = physical_devices[chosen];

	{
		uint32_t present_mode_count;
		vkAssert(Vk::ext.vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[chosen], m_surface, &present_mode_count, nullptr));
		VkPresentModeKHR present_modes[present_mode_count];
		vkAssert(Vk::ext.vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[chosen], m_surface, &present_mode_count, present_modes));
		m_present_mode = present_modes[0];
		bool has_mailbox = false;
		bool has_immediate = false;
		for (size_t i = 0; i < present_mode_count; i++) {
			if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
				has_mailbox = true;
			if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
				has_immediate = true;
		}
		if (has_mailbox)
			m_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
		else if (has_immediate)
			m_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

		uint32_t surface_format_count;
		vkAssert(Vk::ext.vkGetPhysicalDeviceSurfaceFormatsKHR(physical_devices[chosen], m_surface, &surface_format_count, nullptr));
		VkSurfaceFormatKHR surface_formats[surface_format_count];
		vkAssert(Vk::ext.vkGetPhysicalDeviceSurfaceFormatsKHR(physical_devices[chosen], m_surface, &surface_format_count, surface_formats));
		m_surface_format = surface_formats[0];
		for (size_t i = 0; i < surface_format_count; i++)
			if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) {
				m_surface_format = surface_formats[i];
				break;
			}

		vkAssert(Vk::ext.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_devices[chosen], m_surface, &m_surface_capabilities));
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

VkFormat Renderer::getFormatDepth(void)
{
	VkFormatProperties d24_props;
	vkGetPhysicalDeviceFormatProperties(m_physical_device, VK_FORMAT_X8_D24_UNORM_PACK32, &d24_props);
	auto d24_support = (d24_props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) &&
	(d24_props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

	VkFormatProperties d32_props;
	vkGetPhysicalDeviceFormatProperties(m_physical_device, VK_FORMAT_D32_SFLOAT, &d32_props);
	auto d32_support = (d32_props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) &&
	(d24_props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

	if (d24_support)
		return VK_FORMAT_X8_D24_UNORM_PACK32;
	else if (d32_support)
		return VK_FORMAT_D32_SFLOAT;
	else
		throw std::runtime_error("No compatible depth format found");
}

VkSampleCountFlags Renderer::getSupportedSampleCounts(void)
{
	VkFormat formats[] {
		VK_FORMAT_X8_D24_UNORM_PACK32,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_R16G16B16A16_SFLOAT
	};
	VkSampleCountFlags res;

	for (size_t i = 0; i < array_size(formats); i++) {
		auto cur = formats[i];
		VkImageFormatProperties props;
		vkGetPhysicalDeviceImageFormatProperties(m_physical_device, cur, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, 0, &props);
		if (i == 0)
			res = props.sampleCounts;
		else
			res &= props.sampleCounts;
	}
	return res;
}

VkSampleCountFlagBits Renderer::fitSampleCount(VkSampleCountFlagBits sampleCount) const
{
	while (!(sampleCount & m_supported_sample_counts)) {
		sampleCount = static_cast<VkSampleCountFlagBits>(sampleCount >> 1);
		if (sampleCount == 0)
			throw std::runtime_error("Sample count of 1 unsupported");
	}
	return sampleCount;
}

Vk::Allocator Renderer::createAllocator(void)
{
	VmaAllocatorCreateInfo ci{};
	ci.vulkanApiVersion = VK_API_VERSION_1_0;
	ci.physicalDevice = m_physical_device;
	ci.device = device;
	ci.instance = m_instance;
	return Vk::createAllocator(ci);
}

uint32_t Renderer::extentLog2(uint32_t val)
{
	uint32_t res = 0;
	for (uint32_t i = 1; i < val; i <<= static_cast<uint32_t>(1), res++);
	return res;
}

uint32_t Renderer::extentMipLevels(const VkExtent2D &extent)
{
	return static_cast<uint32_t>(std::floor(std::log2(max(extent.width, extent.height)))) + static_cast<uint32_t>(1);
}

uint32_t Renderer::nextExtentMip(uint32_t extent)
{
	return max(extent / static_cast<uint32_t>(2), static_cast<uint32_t>(1));
}

Vk::SwapchainKHR Renderer::createSwapchain(void)
{
	while (true) {
		vkAssert(Vk::ext.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &m_surface_capabilities));

		auto wins = getWindowSize();
		m_swapchain_extent = VkExtent2D{clamp(static_cast<uint32_t>(wins.x), max(m_surface_capabilities.minImageExtent.width, static_cast<uint32_t>(1)), m_surface_capabilities.maxImageExtent.width),
			clamp(static_cast<uint32_t>(wins.y), max(m_surface_capabilities.minImageExtent.height, static_cast<uint32_t>(1)), m_surface_capabilities.maxImageExtent.height)};
		if (m_swapchain_extent.width * m_swapchain_extent.height > 0)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(16));

		pollEvents();
	}

	auto wp = extentLog2(m_swapchain_extent.width);
	auto hp = extentLog2(m_swapchain_extent.height);
	m_swapchain_extent_mip = VkExtent2D{static_cast<uint32_t>(1) << wp, static_cast<uint32_t>(1) << hp};
	m_swapchain_mip_levels = extentMipLevels(m_swapchain_extent_mip);
	m_swapchain_extent_mips.resize(m_swapchain_mip_levels);
	{
		uint32_t w = m_swapchain_extent_mip.width;
		uint32_t h = m_swapchain_extent_mip.height;
		for (uint32_t i = 0; i < m_swapchain_mip_levels; i++) {
			m_swapchain_extent_mips[i] = VkExtent2D{w, h};
			w = nextExtentMip(w);
			h = nextExtentMip(h);
		}
	}

	m_pipeline_viewport_state.viewport = VkViewport{0.0f, 0.0f, static_cast<float>(m_swapchain_extent.width), static_cast<float>(m_swapchain_extent.height), 0.0f, 1.0f};
	m_pipeline_viewport_state.scissor = VkRect2D{{0, 0}, {m_swapchain_extent.width, m_swapchain_extent.height}};
	m_pipeline_viewport_state.ci = VkPipelineViewportStateCreateInfo{};
	m_pipeline_viewport_state.ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_pipeline_viewport_state.ci.viewportCount = 1;
	m_pipeline_viewport_state.ci.pViewports = &m_pipeline_viewport_state.viewport;
	m_pipeline_viewport_state.ci.scissorCount = 1;
	m_pipeline_viewport_state.ci.pScissors = &m_pipeline_viewport_state.scissor;

	VkSwapchainCreateInfoKHR ci{};
	ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	ci.surface = m_surface;
	ci.minImageCount = clamp(m_frame_count, m_surface_capabilities.minImageCount, m_surface_capabilities.maxImageCount);
	ci.imageFormat = m_surface_format.format;
	ci.imageColorSpace = m_surface_format.colorSpace;
	ci.imageExtent = m_swapchain_extent;
	ci.imageArrayLayers = 1;
	ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ci.queueFamilyIndexCount = 1;
	ci.pQueueFamilyIndices = &m_queue_family_graphics;
	ci.preTransform = m_surface_capabilities.currentTransform;
	ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	ci.presentMode = m_present_mode;
	ci.clipped = VK_TRUE;

	return device.createSwapchainKHR(ci);
}

Vk::ImageView Renderer::createImageView(VkImage image, VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspect)
{
	VkImageViewCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ci.image = image;
	ci.viewType = viewType;
	ci.format = format;
	ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	ci.subresourceRange.aspectMask = aspect;
	ci.subresourceRange.baseMipLevel = 0;
	ci.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	ci.subresourceRange.baseArrayLayer = 0;
	ci.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	return device.createImageView(ci);
}

Vk::ImageView Renderer::createImageViewMip(VkImage image, VkImageViewType viewType, VkFormat format, VkImageAspectFlags aspect,
	uint32_t baseMipLevel, uint32_t levelCount)
{
	VkImageViewCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	ci.image = image;
	ci.viewType = viewType;
	ci.format = format;
	ci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	ci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	ci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	ci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	ci.subresourceRange.aspectMask = aspect;
	ci.subresourceRange.baseMipLevel = baseMipLevel;
	ci.subresourceRange.levelCount = levelCount;
	ci.subresourceRange.baseArrayLayer = 0;
	ci.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	return device.createImageView(ci);
}

vector<Vk::ImageView> Renderer::createSwapchainImageViews(void)
{
	vector<Vk::ImageView> res;
	res.reserve(m_swapchain_images.size());
	for (auto &i : m_swapchain_images)
		res.emplace(createImageView(i, VK_IMAGE_VIEW_TYPE_2D, m_surface_format.format, VK_IMAGE_ASPECT_COLOR_BIT));
	return res;
}

Vk::DescriptorSetLayout Renderer::createDescriptorSetLayout0(void)
{
	VkDescriptorSetLayoutCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	VkDescriptorSetLayoutBinding bindings[] {
		{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, s0_sampler_count, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
	};
	ci.bindingCount = array_size(bindings);
	ci.pBindings = bindings;
	return device.createDescriptorSetLayout(ci);
}

Vk::DescriptorSetLayout Renderer::createDescriptorSetLayoutDynamic(void)
{
	VkDescriptorSetLayoutCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	VkDescriptorSetLayoutBinding bindings[] {
		{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}
	};
	ci.bindingCount = array_size(bindings);
	ci.pBindings = bindings;
	return device.createDescriptorSetLayout(ci);
}

Vk::PipelineLayout Renderer::createPipelineLayoutDescriptorSet(void)
{
	VkPipelineLayoutCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	VkDescriptorSetLayout set_layouts[] {
		m_descriptor_set_layout_0,
		m_descriptor_set_layout_dynamic
	};
	ci.setLayoutCount = array_size(set_layouts);
	ci.pSetLayouts = set_layouts;
	VkPushConstantRange ranges[] {
		{VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16}
	};
	ci.pushConstantRangeCount = array_size(ranges);
	ci.pPushConstantRanges = ranges;
	return device.createPipelineLayout(ci);
}

Vk::BufferAllocation Renderer::createScreenVertexBuffer(void)
{
	Vertex::p2 vertices[] {
		{{-1.0f, -1.0f}},
		{{3.0f, -1.0f}},
		{{-1.0f, 3.0f}}
	};
	auto res = createVertexBuffer(sizeof(vertices));
	{
		VkBufferCreateInfo bci{};
		bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bci.size = sizeof(vertices);
		bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VmaAllocationCreateInfo aci{};
		aci.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		aci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		void *data;
		auto s = allocator.createBuffer(bci, aci, &data);
		std::memcpy(data, vertices, sizeof(vertices));
		allocator.invalidateAllocation(s, 0, sizeof(vertices));

		m_transfer_cmd.beginPrimary(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VkBufferCopy region;
		region.srcOffset = 0;
		region.dstOffset = 0;
		region.size = sizeof(vertices);
		m_transfer_cmd.copyBuffer(s, res, 1, &region);
		m_transfer_cmd.end();

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = m_transfer_cmd.ptr();
		m_queue.submit(1, &submit, VK_NULL_HANDLE);
		m_queue.waitIdle();

		allocator.destroy(s);
	}
	return res;
}

Vk::RenderPass Renderer::createOpaquePass(void)
{
	VkRenderPassCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	VkAttachmentDescription atts[] {
		{0, format_depth, m_sample_count, Vk::AttachmentLoadOp::Clear, Vk::AttachmentStoreOp::Store,	// depth 0
			Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
			Vk::ImageLayout::Undefined, Vk::ImageLayout::DepthStencilReadOnlyOptimal},
		{0, VK_FORMAT_R32_SFLOAT, m_sample_count, Vk::AttachmentLoadOp::Clear, Vk::AttachmentStoreOp::Store,	// cdepth 1
			Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
			Vk::ImageLayout::Undefined, Vk::ImageLayout::ShaderReadOnlyOptimal},
		{0, VK_FORMAT_R8G8B8A8_SRGB, m_sample_count, Vk::AttachmentLoadOp::Clear, Vk::AttachmentStoreOp::Store,	// albedo 2
			Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
			Vk::ImageLayout::Undefined, Vk::ImageLayout::ShaderReadOnlyOptimal},
		{0, VK_FORMAT_R16G16B16A16_SFLOAT, m_sample_count, Vk::AttachmentLoadOp::Clear, Vk::AttachmentStoreOp::Store,	// normal 3
			Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
			Vk::ImageLayout::Undefined, Vk::ImageLayout::ShaderReadOnlyOptimal}
	};
	VkAttachmentReference depth {0, Vk::ImageLayout::DepthStencilAttachmentOptimal};
	VkAttachmentReference cdepth {1, Vk::ImageLayout::ColorAttachmentOptimal};
	VkAttachmentReference albedo {2, Vk::ImageLayout::ColorAttachmentOptimal};
	VkAttachmentReference normal {3, Vk::ImageLayout::ColorAttachmentOptimal};

	VkAttachmentReference color_atts[] {
		cdepth,
		albedo,
		normal
	};

	VkSubpassDescription subpasses[] {
		{0, VK_PIPELINE_BIND_POINT_GRAPHICS,
			0, nullptr,		// input
			array_size(color_atts), color_atts, nullptr,	// color, resolve
			&depth,			// depth
			0, nullptr}		// preserve
	};

	ci.attachmentCount = array_size(atts);
	ci.pAttachments = atts;
	ci.subpassCount = array_size(subpasses);
	ci.pSubpasses = subpasses;

	return device.createRenderPass(ci);
}

const Renderer::IllumTechnique::Props& Renderer::getIllumTechniqueProps(void)
{
	static const IllumTechnique::Props props[IllumTechnique::MaxEnum] {
		{
			.descriptorCombinedImageSamplerCount = IllumTechnique::Data::Potato::descriptorCombinedImageSamplerCount,
			.fragShaderPath = "sha/potato",
			.fragShaderColorAttachmentCount = 1,
			.barrsPerFrame = IllumTechnique::Data::Potato::barrsPerFrame,
			.addBarrsPerFrame = IllumTechnique::Data::Potato::addBarrsPerFrame
		},
		{
			.descriptorCombinedImageSamplerCount = IllumTechnique::Data::Ssgi::descriptorCombinedImageSamplerCount,
			.fragShaderPath = "sha/ssgi",
			.fragShaderColorAttachmentCount = 7,
			.barrsPerFrame = IllumTechnique::Data::Ssgi::barrsPerFrame,
			.addBarrsPerFrame = IllumTechnique::Data::Ssgi::addBarrsPerFrame
		}
	};
	static const IllumTechnique::Props props_ms[IllumTechnique::MaxEnum] {
		{
			.descriptorCombinedImageSamplerCount = IllumTechnique::Data::Potato::descriptorCombinedImageSamplerCount,
			.fragShaderPath = "sha/potato_ms",
			.fragShaderColorAttachmentCount = 1,
			.barrsPerFrame = IllumTechnique::Data::Potato::barrsPerFrame,
			.addBarrsPerFrame = IllumTechnique::Data::Potato::addBarrsPerFrame
		},
		{
			.descriptorCombinedImageSamplerCount = IllumTechnique::Data::Ssgi::msDescriptorCombinedImageSamplerCount,
			.fragShaderPath = "sha/ssgi_ms",
			.fragShaderColorAttachmentCount = 7,
			.barrsPerFrame = IllumTechnique::Data::Ssgi::msBarrsPerFrame,
			.addBarrsPerFrame = IllumTechnique::Data::Ssgi::msAddBarrsPerFrame
		}
	};

	if (m_sample_count == VK_SAMPLE_COUNT_1_BIT)
		return props[m_illum_technique];
	else
		return props_ms[m_illum_technique];
}

Vk::RenderPass Renderer::createColorResolvePass(void)
{
	VkRenderPassCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	VkAttachmentDescription atts[] {
		{0, VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLE_COUNT_1_BIT, Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::Store,	// albedo 0
			Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
			Vk::ImageLayout::Undefined, Vk::ImageLayout::ShaderReadOnlyOptimal},
		{0, VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::Store,	// normal 1
			Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
			Vk::ImageLayout::Undefined, Vk::ImageLayout::ShaderReadOnlyOptimal}
	};
	VkAttachmentReference albedo {0, Vk::ImageLayout::ColorAttachmentOptimal};
	VkAttachmentReference normal {1, Vk::ImageLayout::ColorAttachmentOptimal};

	VkAttachmentReference color_atts[] {
		albedo,
		normal
	};

	VkSubpassDescription subpasses[] {
		{0, VK_PIPELINE_BIND_POINT_GRAPHICS,
			0, nullptr,		// input
			array_size(color_atts), color_atts, nullptr,	// color, resolve
			nullptr,			// depth
			0, nullptr}		// preserve
	};

	ci.attachmentCount = array_size(atts);
	ci.pAttachments = atts;
	ci.subpassCount = array_size(subpasses);
	ci.pSubpasses = subpasses;

	return device.createRenderPass(ci);
}

Vk::DescriptorSetLayout Renderer::createColorResolveSetLayout(void)
{
	VkDescriptorSetLayoutCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	VkDescriptorSetLayoutBinding bindings[] {
		{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// depth_buffer
		{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// albedo
		{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// normal
	};
	ci.bindingCount = array_size(bindings);
	ci.pBindings = bindings;
	return device.createDescriptorSetLayout(ci);
}

Pipeline Renderer::createColorResolvePipeline(void)
{
	Pipeline res;

	VkGraphicsPipelineCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	auto frag = loadShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "sha/color_resolve");
	res.pushShaderModule(frag);
	struct FragSpec {
		int32_t sample_count;
	} frag_spec_data{static_cast<int32_t>(m_sample_count)};
	VkSpecializationMapEntry frag_spec_entries[] {
		{0, offsetof(FragSpec, sample_count), sizeof(FragSpec::sample_count)}
	};
	VkSpecializationInfo frag_spec;
	frag_spec.mapEntryCount = array_size(frag_spec_entries);
	frag_spec.pMapEntries = frag_spec_entries;
	frag_spec.dataSize = sizeof(FragSpec);
	frag_spec.pData = &frag_spec_data;
	VkPipelineShaderStageCreateInfo stages[] {
		initPipelineStage(VK_SHADER_STAGE_VERTEX_BIT, m_fwd_p2_module),
		VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
			VK_SHADER_STAGE_FRAGMENT_BIT, frag, "main", &frag_spec}
	};
	ci.stageCount = array_size(stages);
	ci.pStages = stages;

	VkPipelineVertexInputStateCreateInfo vertex_input{};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VkVertexInputBindingDescription vertex_input_bindings[] {
		{0, sizeof(Vertex::p2), VK_VERTEX_INPUT_RATE_VERTEX}
	};
	vertex_input.vertexBindingDescriptionCount = array_size(vertex_input_bindings);
	vertex_input.pVertexBindingDescriptions = vertex_input_bindings;
	VkVertexInputAttributeDescription vertex_input_attributes[] {
		{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex::p2, p)}
	};
	vertex_input.vertexAttributeDescriptionCount = array_size(vertex_input_attributes);
	vertex_input.pVertexAttributeDescriptions = vertex_input_attributes;
	ci.pVertexInputState = &vertex_input;

	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	ci.pInputAssemblyState = &input_assembly;

	ci.pViewportState = &m_pipeline_viewport_state.ci;

	VkPipelineRasterizationStateCreateInfo rasterization{};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization.lineWidth = 1.0f;
	ci.pRasterizationState = &rasterization;

	VkPipelineMultisampleStateCreateInfo multisample{};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	ci.pMultisampleState = &multisample;

	VkPipelineColorBlendStateCreateInfo color_blend{};
	color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendAttachmentState color_blend_attachments[] {
		color_blend_attachment,
		color_blend_attachment
	};
	color_blend.attachmentCount = array_size(color_blend_attachments);
	color_blend.pAttachments = color_blend_attachments;
	ci.pColorBlendState = &color_blend;

	VkPipelineDynamicStateCreateInfo dynamic{};
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	VkDynamicState dynamic_states[] {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	dynamic.dynamicStateCount = array_size(dynamic_states);
	dynamic.pDynamicStates = dynamic_states;
	ci.pDynamicState = &dynamic;

	{
		VkPipelineLayoutCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		VkDescriptorSetLayout set_layouts[] {
			m_color_resolve_set_layout
		};
		ci.setLayoutCount = array_size(set_layouts);
		ci.pSetLayouts = set_layouts;
		res.pipelineLayout = device.createPipelineLayout(ci);
	}
	ci.layout = res.pipelineLayout;
	ci.renderPass = m_color_resolve_pass;

	res = device.createGraphicsPipeline(m_pipeline_cache, ci);
	return res;
}

Vk::RenderPass Renderer::createDepthResolvePass(void)
{
	VkRenderPassCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	VkAttachmentDescription atts[] {
		{0, VK_FORMAT_R32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::Store,	// depth 0
			Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
			Vk::ImageLayout::Undefined, Vk::ImageLayout::ShaderReadOnlyOptimal}
	};
	VkAttachmentReference depth {0, Vk::ImageLayout::ColorAttachmentOptimal};

	VkAttachmentReference color_atts[] {
		depth
	};

	VkSubpassDescription subpasses[] {
		{0, VK_PIPELINE_BIND_POINT_GRAPHICS,
			0, nullptr,		// input
			array_size(color_atts), color_atts, nullptr,	// color, resolve
			nullptr,			// depth
			0, nullptr}		// preserve
	};

	ci.attachmentCount = array_size(atts);
	ci.pAttachments = atts;
	ci.subpassCount = array_size(subpasses);
	ci.pSubpasses = subpasses;

	return device.createRenderPass(ci);
}

Vk::DescriptorSetLayout Renderer::createDepthResolveSetLayout(void)
{
	VkDescriptorSetLayoutCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	VkDescriptorSetLayoutBinding bindings[] {
		{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// depth_buffer
	};
	ci.bindingCount = array_size(bindings);
	ci.pBindings = bindings;
	return device.createDescriptorSetLayout(ci);
}

Pipeline Renderer::createDepthResolvePipeline(void)
{
	Pipeline res;

	VkGraphicsPipelineCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	auto frag = loadShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, m_sample_count == VK_SAMPLE_COUNT_1_BIT ? "sha/depth_resolve" : "sha/depth_resolve_ms");
	res.pushShaderModule(frag);
	struct FragSpec {
		int32_t sample_count;
	} frag_spec_data{static_cast<int32_t>(m_sample_count)};
	VkSpecializationMapEntry frag_spec_entries[] {
		{0, offsetof(FragSpec, sample_count), sizeof(FragSpec::sample_count)}
	};
	VkSpecializationInfo frag_spec;
	frag_spec.mapEntryCount = array_size(frag_spec_entries);
	frag_spec.pMapEntries = frag_spec_entries;
	frag_spec.dataSize = sizeof(FragSpec);
	frag_spec.pData = &frag_spec_data;
	VkPipelineShaderStageCreateInfo stages[] {
		initPipelineStage(VK_SHADER_STAGE_VERTEX_BIT, m_fwd_p2_module),
		VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
			VK_SHADER_STAGE_FRAGMENT_BIT, frag, "main", &frag_spec}
	};
	ci.stageCount = array_size(stages);
	ci.pStages = stages;

	VkPipelineVertexInputStateCreateInfo vertex_input{};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VkVertexInputBindingDescription vertex_input_bindings[] {
		{0, sizeof(Vertex::p2), VK_VERTEX_INPUT_RATE_VERTEX}
	};
	vertex_input.vertexBindingDescriptionCount = array_size(vertex_input_bindings);
	vertex_input.pVertexBindingDescriptions = vertex_input_bindings;
	VkVertexInputAttributeDescription vertex_input_attributes[] {
		{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex::p2, p)}
	};
	vertex_input.vertexAttributeDescriptionCount = array_size(vertex_input_attributes);
	vertex_input.pVertexAttributeDescriptions = vertex_input_attributes;
	ci.pVertexInputState = &vertex_input;

	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	ci.pInputAssemblyState = &input_assembly;

	ci.pViewportState = &m_pipeline_viewport_state.ci;

	VkPipelineRasterizationStateCreateInfo rasterization{};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization.lineWidth = 1.0f;
	ci.pRasterizationState = &rasterization;

	VkPipelineMultisampleStateCreateInfo multisample{};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	ci.pMultisampleState = &multisample;

	VkPipelineColorBlendStateCreateInfo color_blend{};
	color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendAttachmentState color_blend_attachments[] {
		color_blend_attachment
	};
	color_blend.attachmentCount = array_size(color_blend_attachments);
	color_blend.pAttachments = color_blend_attachments;
	ci.pColorBlendState = &color_blend;

	VkPipelineDynamicStateCreateInfo dynamic{};
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	VkDynamicState dynamic_states[] {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	dynamic.dynamicStateCount = array_size(dynamic_states);
	dynamic.pDynamicStates = dynamic_states;
	ci.pDynamicState = &dynamic;

	{
		VkPipelineLayoutCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		VkDescriptorSetLayout set_layouts[] {
			m_depth_resolve_set_layout
		};
		ci.setLayoutCount = array_size(set_layouts);
		ci.pSetLayouts = set_layouts;
		res.pipelineLayout = device.createPipelineLayout(ci);
	}
	ci.layout = res.pipelineLayout;
	ci.renderPass = m_depth_resolve_pass;

	res = device.createGraphicsPipeline(m_pipeline_cache, ci);
	return res;
}

Pipeline Renderer::createDepthAccPipeline(void)
{
	Pipeline res;

	VkGraphicsPipelineCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	auto frag = loadShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "sha/depth_acc");
	res.pushShaderModule(frag);
	VkPipelineShaderStageCreateInfo stages[] {
		initPipelineStage(VK_SHADER_STAGE_VERTEX_BIT, m_fwd_p2_module),
		initPipelineStage(VK_SHADER_STAGE_FRAGMENT_BIT, frag)
	};
	ci.stageCount = array_size(stages);
	ci.pStages = stages;

	VkPipelineVertexInputStateCreateInfo vertex_input{};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VkVertexInputBindingDescription vertex_input_bindings[] {
		{0, sizeof(Vertex::p2), VK_VERTEX_INPUT_RATE_VERTEX}
	};
	vertex_input.vertexBindingDescriptionCount = array_size(vertex_input_bindings);
	vertex_input.pVertexBindingDescriptions = vertex_input_bindings;
	VkVertexInputAttributeDescription vertex_input_attributes[] {
		{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex::p2, p)}
	};
	vertex_input.vertexAttributeDescriptionCount = array_size(vertex_input_attributes);
	vertex_input.pVertexAttributeDescriptions = vertex_input_attributes;
	ci.pVertexInputState = &vertex_input;

	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	ci.pInputAssemblyState = &input_assembly;

	ci.pViewportState = &m_pipeline_viewport_state.ci;

	VkPipelineRasterizationStateCreateInfo rasterization{};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization.lineWidth = 1.0f;
	ci.pRasterizationState = &rasterization;

	VkPipelineMultisampleStateCreateInfo multisample{};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	ci.pMultisampleState = &multisample;

	VkPipelineColorBlendStateCreateInfo color_blend{};
	color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendAttachmentState color_blend_attachments[] {
		color_blend_attachment
	};
	color_blend.attachmentCount = array_size(color_blend_attachments);
	color_blend.pAttachments = color_blend_attachments;
	ci.pColorBlendState = &color_blend;

	VkPipelineDynamicStateCreateInfo dynamic{};
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	VkDynamicState dynamic_states[] {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	dynamic.dynamicStateCount = array_size(dynamic_states);
	dynamic.pDynamicStates = dynamic_states;
	ci.pDynamicState = &dynamic;

	{
		VkPipelineLayoutCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		VkDescriptorSetLayout set_layouts[] {
			m_depth_resolve_set_layout
		};
		ci.setLayoutCount = array_size(set_layouts);
		ci.pSetLayouts = set_layouts;
		res.pipelineLayout = device.createPipelineLayout(ci);
	}
	ci.layout = res.pipelineLayout;
	ci.renderPass = m_depth_resolve_pass;

	res = device.createGraphicsPipeline(m_pipeline_cache, ci);
	return res;
}

Vk::RenderPass Renderer::createIlluminationPass(void)
{
	VkRenderPassCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	if (m_illum_technique == IllumTechnique::Potato) {
		VkAttachmentDescription atts[] {
			{0, VK_FORMAT_R16G16B16A16_SFLOAT, Vk::SampleCount_1Bit, Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::Store,	// output 0
				Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
				Vk::ImageLayout::Undefined, Vk::ImageLayout::ShaderReadOnlyOptimal}
		};
		VkAttachmentReference output {0, Vk::ImageLayout::ColorAttachmentOptimal};

		VkAttachmentReference color_atts[] {
			output
		};

		VkSubpassDescription subpasses[] {
			{0, VK_PIPELINE_BIND_POINT_GRAPHICS,
				0, nullptr,		// input
				array_size(color_atts), color_atts, nullptr,	// color, resolve
				nullptr,			// depth
				0, nullptr}		// preserve
		};

		ci.attachmentCount = array_size(atts);
		ci.pAttachments = atts;
		ci.subpassCount = array_size(subpasses);
		ci.pSubpasses = subpasses;

		return device.createRenderPass(ci);
	}
	if (m_illum_technique == IllumTechnique::Ssgi) {
		VkAttachmentDescription atts[] {
			{0, VK_FORMAT_R8_SINT, Vk::SampleCount_1Bit, Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::Store,	// step 0
				Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
				Vk::ImageLayout::Undefined, Vk::ImageLayout::ShaderReadOnlyOptimal},
			{0, VK_FORMAT_R16G16B16A16_UINT, Vk::SampleCount_1Bit, Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::Store,	// acc 1
				Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
				Vk::ImageLayout::Undefined, Vk::ImageLayout::ShaderReadOnlyOptimal},
			{0, VK_FORMAT_R16G16B16A16_SFLOAT, Vk::SampleCount_1Bit, Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::Store,	// direct_light 2
				Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
				Vk::ImageLayout::Undefined, Vk::ImageLayout::ShaderReadOnlyOptimal},
			{0, VK_FORMAT_R16G16B16A16_SFLOAT, Vk::SampleCount_1Bit, Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::Store,	// path_albedo 3
				Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
				Vk::ImageLayout::Undefined, Vk::ImageLayout::ShaderReadOnlyOptimal},
			{0, VK_FORMAT_R16G16B16A16_SFLOAT, Vk::SampleCount_1Bit, Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::Store,	// path_direct_light 4
				Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
				Vk::ImageLayout::Undefined, Vk::ImageLayout::ShaderReadOnlyOptimal},
			{0, VK_FORMAT_R16G16B16A16_SFLOAT, Vk::SampleCount_1Bit, Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::Store,	// path_incidence 5
				Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
				Vk::ImageLayout::Undefined, Vk::ImageLayout::ShaderReadOnlyOptimal},
			{0, VK_FORMAT_R16G16B16A16_SFLOAT, Vk::SampleCount_1Bit, Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::Store,	// output 6
				Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
				Vk::ImageLayout::Undefined, Vk::ImageLayout::ShaderReadOnlyOptimal}
		};
		VkAttachmentReference step {0, Vk::ImageLayout::ColorAttachmentOptimal};
		VkAttachmentReference acc_path_pos {1, Vk::ImageLayout::ColorAttachmentOptimal};
		VkAttachmentReference direct_light {2, Vk::ImageLayout::ColorAttachmentOptimal};
		VkAttachmentReference path_albedo {3, Vk::ImageLayout::ColorAttachmentOptimal};
		VkAttachmentReference path_direct_light {4, Vk::ImageLayout::ColorAttachmentOptimal};
		VkAttachmentReference path_incidence {5, Vk::ImageLayout::ColorAttachmentOptimal};
		VkAttachmentReference output {6, Vk::ImageLayout::ColorAttachmentOptimal};

		VkAttachmentReference color_atts[] {
			step,
			acc_path_pos,
			direct_light,
			path_albedo,
			path_direct_light,
			path_incidence,
			output
		};

		VkSubpassDescription subpasses[] {
			{0, VK_PIPELINE_BIND_POINT_GRAPHICS,
				0, nullptr,		// input
				array_size(color_atts), color_atts, nullptr,	// color, resolve
				nullptr,			// depth
				0, nullptr}		// preserve
		};

		ci.attachmentCount = array_size(atts);
		ci.pAttachments = atts;
		ci.subpassCount = array_size(subpasses);
		ci.pSubpasses = subpasses;

		return device.createRenderPass(ci);
	}
	throw std::runtime_error("createIlluminationPass");
}

Vk::DescriptorSetLayout Renderer::createIlluminationSetLayout(void)
{
	VkDescriptorSetLayoutCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

	if (m_illum_technique == IllumTechnique::Potato) {
		VkDescriptorSetLayoutBinding bindings[] {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// depth
			{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// albedo
			{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// normal
		};
		ci.bindingCount = array_size(bindings);
		ci.pBindings = bindings;
		return device.createDescriptorSetLayout(ci);
	}
	if (m_illum_technique == IllumTechnique::Ssgi) {
		if (m_sample_count == VK_SAMPLE_COUNT_1_BIT) {
			VkDescriptorSetLayoutBinding bindings[] {
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
				{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// depth
				{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// albedo
				{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// normal
				{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_depth
				{5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_albedo
				{6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_normal
				{7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_step
				{8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_acc_path_pos
				{9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_direct_light
				{10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_path_albedo
				{11, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_path_direct_light
				{12, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_path_incidence
				{13, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_output
			};
			ci.bindingCount = array_size(bindings);
			ci.pBindings = bindings;
			return device.createDescriptorSetLayout(ci);
		} else {
			VkDescriptorSetLayoutBinding bindings[] {
				{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
				{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// depth
				{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// albedo
				{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// normal
				{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// normal
				{5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// normal
				{6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_depth
				{7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_albedo
				{8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_normal
				{9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_step
				{10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_acc
				{11, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_direct_light
				{12, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_path_pos
				{13, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_path_albedo
				{14, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_path_direct_light
				{15, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_path_incidence
				{16, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// last_output
			};
			ci.bindingCount = array_size(bindings);
			ci.pBindings = bindings;
			return device.createDescriptorSetLayout(ci);
		}
	}
	throw std::runtime_error("createIlluminationSetLayout");
}

Pipeline Renderer::createIlluminationPipeline(void)
{
	Pipeline res;

	VkGraphicsPipelineCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	auto frag = loadShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, m_illum_technique_props.fragShaderPath);
	res.pushShaderModule(frag);
	struct FragSpec {
		int32_t sample_count;
		float sample_factor;
	} frag_spec_data{static_cast<int32_t>(m_sample_count), 1.0f / static_cast<float>(m_sample_count)};
	VkSpecializationMapEntry frag_spec_entries[] {
		{0, offsetof(FragSpec, sample_count), sizeof(FragSpec::sample_count)},
		{1, offsetof(FragSpec, sample_factor), sizeof(FragSpec::sample_factor)}
	};
	VkSpecializationInfo frag_spec;
	frag_spec.mapEntryCount = array_size(frag_spec_entries);
	frag_spec.pMapEntries = frag_spec_entries;
	frag_spec.dataSize = sizeof(FragSpec);
	frag_spec.pData = &frag_spec_data;
	VkPipelineShaderStageCreateInfo stages[] {
		initPipelineStage(VK_SHADER_STAGE_VERTEX_BIT, m_fwd_p2_module),
		VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
			VK_SHADER_STAGE_FRAGMENT_BIT, frag, "main", &frag_spec}
	};
	ci.stageCount = array_size(stages);
	ci.pStages = stages;

	VkPipelineVertexInputStateCreateInfo vertex_input{};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VkVertexInputBindingDescription vertex_input_bindings[] {
		{0, sizeof(Vertex::p2), VK_VERTEX_INPUT_RATE_VERTEX}
	};
	vertex_input.vertexBindingDescriptionCount = array_size(vertex_input_bindings);
	vertex_input.pVertexBindingDescriptions = vertex_input_bindings;
	VkVertexInputAttributeDescription vertex_input_attributes[] {
		{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex::p2, p)}
	};
	vertex_input.vertexAttributeDescriptionCount = array_size(vertex_input_attributes);
	vertex_input.pVertexAttributeDescriptions = vertex_input_attributes;
	ci.pVertexInputState = &vertex_input;

	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	ci.pInputAssemblyState = &input_assembly;

	ci.pViewportState = &m_pipeline_viewport_state.ci;

	VkPipelineRasterizationStateCreateInfo rasterization{};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization.lineWidth = 1.0f;
	ci.pRasterizationState = &rasterization;

	VkPipelineMultisampleStateCreateInfo multisample{};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	ci.pMultisampleState = &multisample;

	VkPipelineColorBlendStateCreateInfo color_blend{};
	color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendAttachmentState color_blend_attachments[m_illum_technique_props.fragShaderColorAttachmentCount];
	for (uint32_t i = 0; i < m_illum_technique_props.fragShaderColorAttachmentCount; i++)
		color_blend_attachments[i] = color_blend_attachment;
	color_blend.attachmentCount = m_illum_technique_props.fragShaderColorAttachmentCount;
	color_blend.pAttachments = color_blend_attachments;
	ci.pColorBlendState = &color_blend;

	VkPipelineDynamicStateCreateInfo dynamic{};
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	VkDynamicState dynamic_states[] {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	dynamic.dynamicStateCount = array_size(dynamic_states);
	dynamic.pDynamicStates = dynamic_states;
	ci.pDynamicState = &dynamic;

	{
		VkPipelineLayoutCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		VkDescriptorSetLayout set_layouts[] {
			m_illumination_set_layout
		};
		ci.setLayoutCount = array_size(set_layouts);
		ci.pSetLayouts = set_layouts;
		res.pipelineLayout = device.createPipelineLayout(ci);
	}
	ci.layout = res.pipelineLayout;
	ci.renderPass = m_illumination_pass;

	res = device.createGraphicsPipeline(m_pipeline_cache, ci);
	return res;
}

Vk::RenderPass Renderer::createWsiPass(void)
{
	VkRenderPassCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	VkAttachmentDescription atts[] {
		{0, VK_FORMAT_B8G8R8A8_SRGB, Vk::SampleCount_1Bit, Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::Store,	// wsi 0
			Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
			Vk::ImageLayout::Undefined, Vk::ImageLayout::PresentSrcKhr}
	};
	VkAttachmentReference wsi {0, Vk::ImageLayout::ColorAttachmentOptimal};

	VkAttachmentReference color_atts[] {
		wsi
	};

	VkSubpassDescription subpasses[] {
		{0, VK_PIPELINE_BIND_POINT_GRAPHICS,
			0, nullptr,		// input
			array_size(color_atts), color_atts, nullptr,	// color, resolve
			nullptr,			// depth
			0, nullptr}		// preserve
	};

	ci.attachmentCount = array_size(atts);
	ci.pAttachments = atts;
	ci.subpassCount = array_size(subpasses);
	ci.pSubpasses = subpasses;

	return device.createRenderPass(ci);
}

Vk::DescriptorSetLayout Renderer::createWsiSetLayout(void)
{
	VkDescriptorSetLayoutCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	VkDescriptorSetLayoutBinding bindings[] {
		{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
	};
	ci.bindingCount = array_size(bindings);
	ci.pBindings = bindings;
	return device.createDescriptorSetLayout(ci);
}

Pipeline Renderer::createWsiPipeline(void)
{
	Pipeline res;

	VkGraphicsPipelineCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	auto frag = loadShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "sha/wsi");
	res.pushShaderModule(frag);
	VkPipelineShaderStageCreateInfo stages[] {
		initPipelineStage(VK_SHADER_STAGE_VERTEX_BIT, m_fwd_p2_module),
		initPipelineStage(VK_SHADER_STAGE_FRAGMENT_BIT, frag)
	};
	ci.stageCount = array_size(stages);
	ci.pStages = stages;

	VkPipelineVertexInputStateCreateInfo vertex_input{};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VkVertexInputBindingDescription vertex_input_bindings[] {
		{0, sizeof(Vertex::p2), VK_VERTEX_INPUT_RATE_VERTEX}
	};
	vertex_input.vertexBindingDescriptionCount = array_size(vertex_input_bindings);
	vertex_input.pVertexBindingDescriptions = vertex_input_bindings;
	VkVertexInputAttributeDescription vertex_input_attributes[] {
		{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex::p2, p)}
	};
	vertex_input.vertexAttributeDescriptionCount = array_size(vertex_input_attributes);
	vertex_input.pVertexAttributeDescriptions = vertex_input_attributes;
	ci.pVertexInputState = &vertex_input;

	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	ci.pInputAssemblyState = &input_assembly;

	ci.pViewportState = &m_pipeline_viewport_state.ci;

	VkPipelineRasterizationStateCreateInfo rasterization{};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization.lineWidth = 1.0f;
	ci.pRasterizationState = &rasterization;

	VkPipelineMultisampleStateCreateInfo multisample{};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	ci.pMultisampleState = &multisample;

	VkPipelineColorBlendStateCreateInfo color_blend{};
	color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendAttachmentState color_blend_attachments[] {
		color_blend_attachment,
	};
	color_blend.attachmentCount = array_size(color_blend_attachments);
	color_blend.pAttachments = color_blend_attachments;
	ci.pColorBlendState = &color_blend;

	VkPipelineDynamicStateCreateInfo dynamic{};
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	VkDynamicState dynamic_states[] {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	dynamic.dynamicStateCount = array_size(dynamic_states);
	dynamic.pDynamicStates = dynamic_states;
	ci.pDynamicState = &dynamic;

	{
		VkPipelineLayoutCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		VkDescriptorSetLayout set_layouts[] {
			m_wsi_set_layout
		};
		ci.setLayoutCount = array_size(set_layouts);
		ci.pSetLayouts = set_layouts;
		res.pipelineLayout = device.createPipelineLayout(ci);
	}
	ci.layout = res.pipelineLayout;
	ci.renderPass = m_wsi_pass;

	res = device.createGraphicsPipeline(m_pipeline_cache, ci);
	return res;
}

Vk::Sampler Renderer::createSamplerFb(void)
{
	return device.createSampler(VkSamplerCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.unnormalizedCoordinates = true
	});
}

Vk::Sampler Renderer::createSamplerFbMip(void)
{
	return device.createSampler(VkSamplerCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.minLod = 0.0f,
		.maxLod = VK_LOD_CLAMP_NONE
	});
}

Vk::Sampler Renderer::createSamplerFbLin(void)
{
	return device.createSampler(VkSamplerCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.unnormalizedCoordinates = true
	});
}

Vk::DescriptorPool Renderer::createDescriptorPool(void)
{
	VkDescriptorPoolCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	uint32_t sets_per_frame = const_sets_per_frame +
		(m_illum_technique == IllumTechnique::Ssgi ?
		1 +	// depth_resolve
			(m_sample_count > VK_SAMPLE_COUNT_1_BIT ?
				1	// color_resolved
				: 0)
		: 0);
	ci.maxSets = m_frame_count * sets_per_frame;
	VkDescriptorPoolSize pool_sizes[] {
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, m_frame_count},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_frame_count},	// illum
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_frame_count * (
			s0_sampler_count +	// s0
			m_illum_technique_props.descriptorCombinedImageSamplerCount +			// illumination
			1			// wsi
		)}
	};
	ci.poolSizeCount = array_size(pool_sizes);
	ci.pPoolSizes = pool_sizes;
	return device.createDescriptorPool(ci);
}

Vk::DescriptorPool Renderer::createDescriptorPoolMip(void)
{
	VkDescriptorPoolCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	ci.maxSets = m_frame_count * sets_per_frame_mip * (m_swapchain_mip_levels - 1);
	VkDescriptorPoolSize pool_sizes[] {
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			m_frame_count * sets_per_frame_mip * (m_swapchain_mip_levels - 1)
		}
	};
	ci.poolSizeCount = array_size(pool_sizes);
	ci.pPoolSizes = pool_sizes;
	return device.createDescriptorPool(ci);
}

vector<Renderer::Frame> Renderer::createFrames(void)
{
	uint32_t sets_per_frame = const_sets_per_frame +
		(m_illum_technique == IllumTechnique::Ssgi ?
		1 +	// depth_resolve
			(m_sample_count > VK_SAMPLE_COUNT_1_BIT ?
				1	// color_resolved
				: 0)
		: 0);
	VkCommandBuffer cmds[m_frame_count * sets_per_frame];
	device.allocateCommandBuffers(m_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_frame_count * sets_per_frame, cmds);
	VkDescriptorSet sets[m_frame_count * sets_per_frame];
	VkDescriptorSetLayout set_layouts[m_frame_count * sets_per_frame];
	for (uint32_t i = 0; i < m_frame_count; i++) {
		set_layouts[i * sets_per_frame] = m_descriptor_set_layout_0;
		set_layouts[i * sets_per_frame + 1] = m_descriptor_set_layout_dynamic;
		set_layouts[i * sets_per_frame + 2] = m_illumination_set_layout;
		set_layouts[i * sets_per_frame + 3] = m_wsi_set_layout;
		if (m_illum_technique == IllumTechnique::Ssgi) {
			set_layouts[i * sets_per_frame + 4] = m_depth_resolve_set_layout;
			if (m_sample_count > VK_SAMPLE_COUNT_1_BIT)
				set_layouts[i * sets_per_frame + 5] = m_color_resolve_set_layout;
		}
	}
	device.allocateDescriptorSets(m_descriptor_pool, m_frame_count * sets_per_frame, set_layouts, sets);

	Vk::BufferAllocation dyn_buffers[m_frame_count];
	for (uint32_t i = 0; i < m_frame_count; i++) {
		VkBufferCreateInfo bci{};
		bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bci.size = Frame::dyn_buffer_size;
		bci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		VmaAllocationCreateInfo aci{};
		aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		dyn_buffers[i] = allocator.createBuffer(bci, aci);
	}

	VkWriteDescriptorSet desc_writes[m_frame_count];
	VkDescriptorBufferInfo bi[m_frame_count];
	for (uint32_t i = 0; i < m_frame_count; i++) {
		VkWriteDescriptorSet cur{};
		cur.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		cur.dstSet = sets[i * sets_per_frame + 1];
		cur.dstBinding = 0;
		cur.dstArrayElement = 0;
		cur.descriptorCount = 1;
		cur.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		auto &cbi = bi[i];
		cbi.buffer = dyn_buffers[i];
		cbi.offset = 0;
		cbi.range = VK_WHOLE_SIZE;
		cur.pBufferInfo = &cbi;
		desc_writes[i] = cur;
	}
	device.updateDescriptorSets(m_frame_count, desc_writes, 0, nullptr);

	size_t sets_mip_stride = (m_swapchain_mip_levels - 1) * sets_per_frame_mip;
	size_t sets_mip_size = m_frame_count * sets_mip_stride;
	VkDescriptorSet sets_mip[sets_mip_size];
	VkDescriptorSetLayout set_layouts_mip[sets_mip_size];
	for (size_t i = 0; i < sets_mip_size; i++)
		set_layouts_mip[i] = m_depth_resolve_set_layout;
	device.allocateDescriptorSets(m_descriptor_pool_mip, sets_mip_size, set_layouts_mip, sets_mip);

	vector<Renderer::Frame> res;
	res.reserve(m_frame_count);
	for (uint32_t i = 0; i < m_frame_count; i++)
		res.emplace(*this, i, cmds[i * 2], cmds[i * 2 + 1],
			sets[i * sets_per_frame], sets[i * sets_per_frame + 1],
			m_illum_technique == IllumTechnique::Ssgi && m_sample_count > VK_SAMPLE_COUNT_1_BIT ? sets[i * sets_per_frame + 5] : VK_NULL_HANDLE,
			m_illum_technique == IllumTechnique::Ssgi ? sets[i * sets_per_frame + 4] : VK_NULL_HANDLE,
			sets[i * sets_per_frame + 2],
			sets[i * sets_per_frame + 3],
			&sets_mip[i * sets_mip_stride],
			dyn_buffers[i]);
	return res;
}

Vk::ShaderModule Renderer::loadShaderModule(VkShaderStageFlagBits stage, const char *path) const
{
	size_t path_len = std::strlen(path);
	char path_ex[path_len + 1 + 4 + 1 + 3 + 1];
	std::memcpy(path_ex, path, path_len);
	size_t i = path_len;
	path_ex[i++] = '.';
	const char *ext = nullptr;
	{
		static const struct { VkShaderStageFlagBits stage; const char *ext; } table[] {
			{VK_SHADER_STAGE_FRAGMENT_BIT, "frag"},
			{VK_SHADER_STAGE_VERTEX_BIT, "vert"},
			{VK_SHADER_STAGE_COMPUTE_BIT, "comp"},
			{VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "tese"},
			{VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "tesc"},
			{VK_SHADER_STAGE_GEOMETRY_BIT, "geom"}
		};
		for (size_t i = 0; i < array_size(table); i++)
			if (table[i].stage == stage) {
				ext = table[i].ext;
				break;
			}
	}
	std::memcpy(&path_ex[i], ext, 4);
	i += 4;
	path_ex[i++] = '.';
	std::memcpy(&path_ex[i], "spv", 3);
	i += 3;
	path_ex[i++] = 0;

	VkShaderModuleCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	std::ifstream f(path_ex, std::ios::binary);
	if (!f.good())
		throw std::runtime_error(path_ex);
	std::stringstream ss;
	ss << f.rdbuf();
	auto str = ss.str();
	ci.codeSize = str.size();
	ci.pCode = reinterpret_cast<const uint32_t*>(str.data());

	return device.createShaderModule(ci);
}

VkPipelineShaderStageCreateInfo Renderer::initPipelineStage(VkShaderStageFlagBits stage, VkShaderModule shaderModule)
{
	struct VkPipelineShaderStageCreateInfo_cpp20 {
		VkStructureType                     sType;
		const void*                         pNext;
		VkPipelineShaderStageCreateFlags    flags;
		VkShaderStageFlagBits               stage;
		VkShaderModule                      shaderModule;
		const char*                         pName;
		const VkSpecializationInfo*         pSpecializationInfo;
	};

	VkPipelineShaderStageCreateInfo res{};
	res.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	res.stage = stage;
	reinterpret_cast<VkPipelineShaderStageCreateInfo_cpp20&>(res).shaderModule = shaderModule;
	res.pName = "main";
	return res;
}

void Pipeline::pushShaderModule(VkShaderModule shaderModule)
{
	shaderModules[shaderModuleCount++] = shaderModule;
}

void Pipeline::pushDynamic(cmp_id cmp)
{
	dynamics[dynamicCount++] = cmp;
}

void Pipeline::destroy(Vk::Device device)
{
	device.destroy(*this);
	if (pipelineLayout != VK_NULL_HANDLE)
		device.destroy(pipelineLayout);
	for (uint32_t i = 0; i < shaderModuleCount; i++)
		device.destroy(shaderModules[i]);
}

void Model::destroy(Vk::Allocator allocator)
{
	allocator.destroy(vertexBuffer);
	if (indexType != VK_INDEX_TYPE_NONE_KHR)
		allocator.destroy(indexBuffer);
}

/*Pipeline Renderer::createPipeline(const char *stagesPath, uint32_t pushConstantRange)
{
	Pipeline res;

	VkGraphicsPipelineCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	auto vert = loadShaderModule(VK_SHADER_STAGE_VERTEX_BIT, stagesPath);
	auto frag = loadShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, stagesPath);
	res.pushShaderModule(vert);
	res.pushShaderModule(frag);
	VkPipelineShaderStageCreateInfo stages[] {
		initPipelineStage(VK_SHADER_STAGE_VERTEX_BIT, vert),
		initPipelineStage(VK_SHADER_STAGE_FRAGMENT_BIT, frag)
	};
	ci.stageCount = array_size(stages);
	ci.pStages = stages;

	VkPipelineVertexInputStateCreateInfo vertex_input{};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VkVertexInputBindingDescription vertex_input_bindings[] {
		{0, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX}
	};
	vertex_input.vertexBindingDescriptionCount = array_size(vertex_input_bindings);
	vertex_input.pVertexBindingDescriptions = vertex_input_bindings;
	VkVertexInputAttributeDescription vertex_input_attributes[] {
		{0, 0, VK_FORMAT_R32G32_SFLOAT, 0}
	};
	vertex_input.vertexAttributeDescriptionCount = array_size(vertex_input_attributes);
	vertex_input.pVertexAttributeDescriptions = vertex_input_attributes;
	ci.pVertexInputState = &vertex_input;

	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	ci.pInputAssemblyState = &input_assembly;

	ci.pViewportState = &m_pipeline_viewport_state.ci;

	VkPipelineRasterizationStateCreateInfo rasterization{};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_POINT;
	rasterization.cullMode = VK_CULL_MODE_NONE;
	rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization.lineWidth = 1.0f;
	ci.pRasterizationState = &rasterization;

	VkPipelineMultisampleStateCreateInfo multisample{};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	ci.pMultisampleState = &multisample;

	VkPipelineColorBlendStateCreateInfo color_blend{};
	color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendAttachmentState color_blend_attachments[] {
		color_blend_attachment
	};
	color_blend.attachmentCount = array_size(color_blend_attachments);
	color_blend.pAttachments = color_blend_attachments;
	ci.pColorBlendState = &color_blend;

	VkPipelineDynamicStateCreateInfo dynamic{};
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	VkDynamicState dynamic_states[] {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	dynamic.dynamicStateCount = array_size(dynamic_states);
	dynamic.pDynamicStates = dynamic_states;
	ci.pDynamicState = &dynamic;

	{
		VkPipelineLayoutCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		VkDescriptorSetLayout set_layouts[] {
			m_descriptor_set_layout_0,
			m_descriptor_set_layout_dynamic
		};
		ci.setLayoutCount = array_size(set_layouts);
		ci.pSetLayouts = set_layouts;
		VkPushConstantRange ranges[] {
			{VK_SHADER_STAGE_FRAGMENT_BIT, 0, pushConstantRange}
		};
		if (pushConstantRange > 0) {
			ci.pushConstantRangeCount = array_size(ranges);
			ci.pPushConstantRanges = ranges;
		}
		res.pipelineLayout = device.createPipelineLayout(ci);
		res.pushConstantRange = pushConstantRange;
	}
	ci.layout = res.pipelineLayout;
	ci.renderPass = m_opaque_pass;

	res = device.createGraphicsPipeline(m_pipeline_cache, ci);
	return res;
}*/

Pipeline Renderer::createPipeline3D_pn(const char *stagesPath, uint32_t pushConstantRange)
{
	Pipeline res;

	VkGraphicsPipelineCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	auto vert = loadShaderModule(VK_SHADER_STAGE_VERTEX_BIT, stagesPath);
	auto frag = loadShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, stagesPath);
	res.pushShaderModule(vert);
	res.pushShaderModule(frag);
	struct FragSpec {
		int32_t sampler_count;
	} frag_spec_data{s0_sampler_count};
	VkSpecializationMapEntry frag_spec_entries[] {
		{0, offsetof(FragSpec, sampler_count), sizeof(FragSpec::sampler_count)}
	};
	VkSpecializationInfo frag_spec;
	frag_spec.mapEntryCount = array_size(frag_spec_entries);
	frag_spec.pMapEntries = frag_spec_entries;
	frag_spec.dataSize = sizeof(FragSpec);
	frag_spec.pData = &frag_spec_data;
	VkPipelineShaderStageCreateInfo stages[] {
		initPipelineStage(VK_SHADER_STAGE_VERTEX_BIT, vert),
		VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
			VK_SHADER_STAGE_FRAGMENT_BIT, frag, "main", &frag_spec}
	};
	ci.stageCount = array_size(stages);
	ci.pStages = stages;

	VkPipelineVertexInputStateCreateInfo vertex_input{};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VkVertexInputBindingDescription vertex_input_bindings[] {
		{0, sizeof(Vertex::pn), VK_VERTEX_INPUT_RATE_VERTEX}
	};
	vertex_input.vertexBindingDescriptionCount = array_size(vertex_input_bindings);
	vertex_input.pVertexBindingDescriptions = vertex_input_bindings;
	VkVertexInputAttributeDescription vertex_input_attributes[] {
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex::pn, p)},
		{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex::pn, n)}
	};
	vertex_input.vertexAttributeDescriptionCount = array_size(vertex_input_attributes);
	vertex_input.pVertexAttributeDescriptions = vertex_input_attributes;
	ci.pVertexInputState = &vertex_input;

	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	input_assembly.primitiveRestartEnable = VK_TRUE;
	ci.pInputAssemblyState = &input_assembly;

	ci.pViewportState = &m_pipeline_viewport_state.ci;

	VkPipelineRasterizationStateCreateInfo rasterization{};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization.lineWidth = 1.0f;
	ci.pRasterizationState = &rasterization;

	VkPipelineMultisampleStateCreateInfo multisample{};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = m_sample_count;
	ci.pMultisampleState = &multisample;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	ci.pDepthStencilState = &depthStencil;

	VkPipelineColorBlendStateCreateInfo color_blend{};
	color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendAttachmentState color_blend_attachments[] {
		color_blend_attachment,
		color_blend_attachment,
		color_blend_attachment
	};
	color_blend.attachmentCount = array_size(color_blend_attachments);
	color_blend.pAttachments = color_blend_attachments;
	ci.pColorBlendState = &color_blend;

	VkPipelineDynamicStateCreateInfo dynamic{};
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	VkDynamicState dynamic_states[] {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	dynamic.dynamicStateCount = array_size(dynamic_states);
	dynamic.pDynamicStates = dynamic_states;
	ci.pDynamicState = &dynamic;

	res.pipelineLayout = VK_NULL_HANDLE;
	res.pushConstantRange = pushConstantRange;
	ci.layout = m_pipeline_layout_descriptor_set;
	ci.renderPass = m_opaque_pass;

	res = device.createGraphicsPipeline(m_pipeline_cache, ci);
	return res;
}

Pipeline Renderer::createPipeline3D_pnu(const char *stagesPath, uint32_t pushConstantRange)
{
	Pipeline res;

	VkGraphicsPipelineCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	auto vert = loadShaderModule(VK_SHADER_STAGE_VERTEX_BIT, stagesPath);
	auto frag = loadShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, stagesPath);
	res.pushShaderModule(vert);
	res.pushShaderModule(frag);
	struct FragSpec {
		int32_t sampler_count;
	} frag_spec_data{s0_sampler_count};
	VkSpecializationMapEntry frag_spec_entries[] {
		{0, offsetof(FragSpec, sampler_count), sizeof(FragSpec::sampler_count)}
	};
	VkSpecializationInfo frag_spec;
	frag_spec.mapEntryCount = array_size(frag_spec_entries);
	frag_spec.pMapEntries = frag_spec_entries;
	frag_spec.dataSize = sizeof(FragSpec);
	frag_spec.pData = &frag_spec_data;
	VkPipelineShaderStageCreateInfo stages[] {
		initPipelineStage(VK_SHADER_STAGE_VERTEX_BIT, vert),
		VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
			VK_SHADER_STAGE_FRAGMENT_BIT, frag, "main", &frag_spec}
	};
	ci.stageCount = array_size(stages);
	ci.pStages = stages;

	VkPipelineVertexInputStateCreateInfo vertex_input{};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VkVertexInputBindingDescription vertex_input_bindings[] {
		{0, sizeof(Vertex::pnu), VK_VERTEX_INPUT_RATE_VERTEX}
	};
	vertex_input.vertexBindingDescriptionCount = array_size(vertex_input_bindings);
	vertex_input.pVertexBindingDescriptions = vertex_input_bindings;
	VkVertexInputAttributeDescription vertex_input_attributes[] {
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex::pnu, p)},
		{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex::pnu, n)},
		{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex::pnu, u)}
	};
	vertex_input.vertexAttributeDescriptionCount = array_size(vertex_input_attributes);
	vertex_input.pVertexAttributeDescriptions = vertex_input_attributes;
	ci.pVertexInputState = &vertex_input;

	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	ci.pInputAssemblyState = &input_assembly;

	ci.pViewportState = &m_pipeline_viewport_state.ci;

	VkPipelineRasterizationStateCreateInfo rasterization{};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization.lineWidth = 1.0f;
	ci.pRasterizationState = &rasterization;

	VkPipelineMultisampleStateCreateInfo multisample{};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = m_sample_count;
	ci.pMultisampleState = &multisample;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	ci.pDepthStencilState = &depthStencil;

	VkPipelineColorBlendStateCreateInfo color_blend{};
	color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendAttachmentState color_blend_attachments[] {
		color_blend_attachment,
		color_blend_attachment,
		color_blend_attachment
	};
	color_blend.attachmentCount = array_size(color_blend_attachments);
	color_blend.pAttachments = color_blend_attachments;
	ci.pColorBlendState = &color_blend;

	VkPipelineDynamicStateCreateInfo dynamic{};
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	VkDynamicState dynamic_states[] {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	dynamic.dynamicStateCount = array_size(dynamic_states);
	dynamic.pDynamicStates = dynamic_states;
	ci.pDynamicState = &dynamic;

	res.pipelineLayout = VK_NULL_HANDLE;
	res.pushConstantRange = pushConstantRange;
	ci.layout = m_pipeline_layout_descriptor_set;
	ci.renderPass = m_opaque_pass;

	res = device.createGraphicsPipeline(m_pipeline_cache, ci);
	return res;
}

Vk::BufferAllocation Renderer::createVertexBuffer(size_t size)
{
	VkBufferCreateInfo bci{};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.size = size;
	bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	VmaAllocationCreateInfo aci{};
	aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	return allocator.createBuffer(bci, aci);
}

Vk::BufferAllocation Renderer::createIndexBuffer(size_t size)
{
	VkBufferCreateInfo bci{};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.size = size;
	bci.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	VmaAllocationCreateInfo aci{};
	aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	return allocator.createBuffer(bci, aci);
}

void Renderer::loadBuffer(VkBuffer buffer, size_t size, const void *data)
{
	VkBufferCreateInfo bci{};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.size = size;
	bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VmaAllocationCreateInfo aci{};
	aci.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	aci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	void *mdata;
	auto s = allocator.createBuffer(bci, aci, &mdata);
	std::memcpy(mdata, data, size);
	allocator.invalidateAllocation(s, 0, size);

	m_transfer_cmd.beginPrimary(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VkBufferCopy region;
	region.srcOffset = 0;
	region.dstOffset = 0;
	region.size = size;
	m_transfer_cmd.copyBuffer(s, buffer, 1, &region);
	m_transfer_cmd.end();

	VkSubmitInfo submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = m_transfer_cmd.ptr();
	m_queue.submit(1, &submit, VK_NULL_HANDLE);
	m_queue.waitIdle();

	allocator.destroy(s);
}

Model Renderer::loadModel(const char *path)
{
	std::ifstream file(path);
	if (!file.good())
		throw std::runtime_error(path);
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &file)) {
		if (err.size() > 0)
			std::cerr << "ERROR: " << path << ": " << err << std::endl;
		throw std::runtime_error(path);
	}
	//if (warn.size() > 0)
	//	std::cout << "WARNING: " << path << ": " << warn << std::endl;
	if (err.size() > 0)
		std::cerr << "ERROR: " << path << ": " << err << std::endl;
	std::vector<Vertex::pnu> vertices;
	for (auto &shape : shapes) {
		size_t ndx = 0;
		for (auto &vert_count : shape.mesh.num_face_vertices) {
			std::vector<glm::vec3> pos;
			std::vector<glm::vec3> normal;
			std::vector<glm::vec2> uv;
			for (size_t i = 0; i < vert_count; i++) {
				auto indices = shape.mesh.indices.at(ndx++);
				if (indices.vertex_index >= 0)
					pos.emplace_back(attrib.vertices.at(indices.vertex_index * 3), attrib.vertices.at(indices.vertex_index * 3 + 1), attrib.vertices.at(indices.vertex_index * 3 + 2));
				if (indices.normal_index >= 0)
					normal.emplace_back(attrib.normals.at(indices.normal_index * 3), attrib.normals.at(indices.normal_index * 3 + 1), attrib.normals.at(indices.normal_index * 3 + 2));
				if (indices.texcoord_index >= 0)
					uv.emplace_back(attrib.texcoords.at(indices.texcoord_index * 2), attrib.texcoords.at(indices.texcoord_index * 2 + 1));
			}

			while (pos.size() < 3)
				pos.emplace_back(0.0);

			if (normal.size() != 3) {
				normal.clear();
				glm::vec3 comp_normal = glm::normalize(glm::cross(pos.at(1) - pos.at(0), pos.at(2) - pos.at(0)));
				for (size_t i = 0; i < 3; i++)
					normal.emplace_back(comp_normal);
			}

			while (uv.size() < 3)
				uv.emplace_back(0.0);

			for (size_t i = 0; i < pos.size(); i++) {
				decltype(vertices)::value_type vertex;
				vertex.p = pos.at(i);
				vertex.n = normal.at(i);
				vertex.u = uv.at(i);
				vertices.emplace_back(vertex);
			}
		}
	}
	Model res;
	res.primitiveCount = vertices.size();
	size_t buf_size = vertices.size() * sizeof(decltype(vertices)::value_type);
	res.vertexBuffer = createVertexBuffer(buf_size);
	res.indexType = VK_INDEX_TYPE_NONE_KHR;
	{
		VkBufferCreateInfo bci{};
		bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bci.size = buf_size;
		bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VmaAllocationCreateInfo aci{};
		aci.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		aci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		void *data;
		auto s = allocator.createBuffer(bci, aci, &data);
		std::memcpy(data, vertices.data(), buf_size);
		allocator.invalidateAllocation(s, 0, buf_size);

		m_transfer_cmd.beginPrimary(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VkBufferCopy region;
		region.srcOffset = 0;
		region.dstOffset = 0;
		region.size = buf_size;
		m_transfer_cmd.copyBuffer(s, res.vertexBuffer, 1, &region);
		m_transfer_cmd.end();

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = m_transfer_cmd.ptr();
		m_queue.submit(1, &submit, VK_NULL_HANDLE);
		m_queue.waitIdle();

		allocator.destroy(s);
	}
	return res;
}

Vk::ImageAllocation Renderer::loadImage(const char *path, bool gen_mips)
{
	int x, y, chan;
	auto data = stbi_load(path, &x, &y, &chan, 4);
	if (data == nullptr)
		throw std::runtime_error(path);
	if (chan != 4)
		throw std::runtime_error(path);

	VkImageCreateInfo ici{};
	ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ici.imageType = VK_IMAGE_TYPE_2D;
	ici.format = VK_FORMAT_R8G8B8A8_SRGB;
	auto extent = VkExtent3D{static_cast<uint32_t>(x), static_cast<uint32_t>(y), 1};
	ici.extent = extent;
	ici.mipLevels = gen_mips ? extentMipLevels(VkExtent2D{extent.width, extent.height}) : 1;
	ici.arrayLayers = 1;
	ici.samples = VK_SAMPLE_COUNT_1_BIT;
	ici.tiling = VK_IMAGE_TILING_OPTIMAL;
	ici.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	VmaAllocationCreateInfo aci{};
	aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	auto res = allocator.createImage(ici, aci);

	{
		size_t buf_size = x * y * chan;
		VkBufferCreateInfo bci{};
		bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bci.size = buf_size;
		bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VmaAllocationCreateInfo aci{};
		aci.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		aci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		void *bdata;
		auto s = allocator.createBuffer(bci, aci, &bdata);
		std::memcpy(bdata, data, buf_size);
		stbi_image_free(data);
		allocator.invalidateAllocation(s, 0, buf_size);

		m_transfer_cmd.beginPrimary(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		{
			VkImageMemoryBarrier ibarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, 0, Vk::Access::TransferReadBit,
				Vk::ImageLayout::Undefined, Vk::ImageLayout::TransferDstOptimal, 0, 0, res,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS } };
			m_transfer_cmd.pipelineBarrier(Vk::PipelineStage::BottomOfPipeBit, Vk::PipelineStage::TransferBit, 0,
				0, nullptr, 0, nullptr, 1, &ibarrier);
		}
		{
			VkBufferImageCopy region{};
			region.imageSubresource = VkImageSubresourceLayers { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			region.imageOffset = VkOffset3D{0, 0, 0};
			region.imageExtent = extent;
			m_transfer_cmd.copyBufferToImage(s, res, Vk::ImageLayout::TransferDstOptimal, 1, &region);
		}
		{
			VkImageMemoryBarrier ibarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, Vk::Access::TransferWriteBit, Vk::Access::TransferReadBit,
				Vk::ImageLayout::TransferDstOptimal, Vk::ImageLayout::TransferSrcOptimal, 0, 0, res,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS } };
			m_transfer_cmd.pipelineBarrier(Vk::PipelineStage::TransferBit, Vk::PipelineStage::TransferBit, 0,
				0, nullptr, 0, nullptr, 1, &ibarrier);
		}
		auto cur_size = VkExtent2D{extent.width, extent.height};
		for (uint32_t i = 0; i < ici.mipLevels - 1; i++) {
			{
				VkImageMemoryBarrier ibarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, Vk::Access::TransferWriteBit, Vk::Access::TransferReadBit,
					Vk::ImageLayout::Undefined, Vk::ImageLayout::TransferDstOptimal, 0, 0, res,
					{ VK_IMAGE_ASPECT_COLOR_BIT, i + 1, 1, 0, VK_REMAINING_ARRAY_LAYERS } };
				m_transfer_cmd.pipelineBarrier(Vk::PipelineStage::TransferBit, Vk::PipelineStage::TransferBit, 0,
					0, nullptr, 0, nullptr, 1, &ibarrier);
			}

			VkImageBlit region;
			region.srcSubresource.aspectMask = Vk::ImageAspect::ColorBit;
			region.srcSubresource.mipLevel = i;
			region.srcSubresource.baseArrayLayer = 0;
			region.srcSubresource.layerCount = 1;
			region.srcOffsets[0] = VkOffset3D{0, 0, 0};
			region.srcOffsets[1] = VkOffset3D{static_cast<int32_t>(cur_size.width), static_cast<int32_t>(cur_size.height), 1};

			cur_size = VkExtent2D{nextExtentMip(cur_size.width), nextExtentMip(cur_size.height)};
			region.dstSubresource.aspectMask = Vk::ImageAspect::ColorBit;
			region.dstSubresource.mipLevel = i + 1;
			region.dstSubresource.baseArrayLayer = 0;
			region.dstSubresource.layerCount = 1;
			region.dstOffsets[0] = VkOffset3D{0, 0, 0};
			region.dstOffsets[1] = VkOffset3D{static_cast<int32_t>(cur_size.width), static_cast<int32_t>(cur_size.height), 1};
			m_transfer_cmd.blitImage(res, Vk::ImageLayout::TransferSrcOptimal, res, Vk::ImageLayout::TransferDstOptimal, 
				1, &region, VK_FILTER_LINEAR);

			{
				VkImageMemoryBarrier ibarriers[] { 
					{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, Vk::Access::TransferWriteBit, Vk::Access::TransferReadBit,
						Vk::ImageLayout::TransferSrcOptimal, Vk::ImageLayout::ShaderReadOnlyOptimal, 0, 0, res,
						{ VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, VK_REMAINING_ARRAY_LAYERS } },
					{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, Vk::Access::TransferWriteBit, Vk::Access::TransferReadBit,
						Vk::ImageLayout::TransferDstOptimal, Vk::ImageLayout::TransferSrcOptimal, 0, 0, res,
						{ VK_IMAGE_ASPECT_COLOR_BIT, i + 1, 1, 0, VK_REMAINING_ARRAY_LAYERS } }
				};
				m_transfer_cmd.pipelineBarrier(Vk::PipelineStage::TransferBit, Vk::PipelineStage::TransferBit, 0,
					0, nullptr, 0, nullptr, array_size(ibarriers), ibarriers);
			}
		}
		{
			VkImageMemoryBarrier ibarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, Vk::Access::TransferWriteBit, Vk::Access::TransferReadBit,
				Vk::ImageLayout::TransferSrcOptimal, Vk::ImageLayout::ShaderReadOnlyOptimal, 0, 0, res,
				{ VK_IMAGE_ASPECT_COLOR_BIT, ici.mipLevels - 1, 1, 0, VK_REMAINING_ARRAY_LAYERS } };
			m_transfer_cmd.pipelineBarrier(Vk::PipelineStage::TransferBit, Vk::PipelineStage::TransferBit, 0,
				0, nullptr, 0, nullptr, 1, &ibarrier);
		}
		m_transfer_cmd.end();

		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = m_transfer_cmd.ptr();
		m_queue.submit(1, &submit, VK_NULL_HANDLE);
		m_queue.waitIdle();

		allocator.destroy(s);
	}
	return res;
}

void Renderer::bindCombinedImageSamplers(uint32_t firstSampler, uint32_t imageInfoCount, const VkDescriptorImageInfo *pImageInfos)
{
	VkWriteDescriptorSet writes[m_frame_count];
	for (size_t i = 0; i < m_frame_count; i++) {
		VkWriteDescriptorSet w{};
		w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		w.dstSet = m_frames[i].m_descriptor_set_0;
		w.dstBinding = 0;
		w.dstArrayElement = firstSampler;
		w.descriptorCount = imageInfoCount;
		w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		w.pImageInfo = pImageInfos;
		writes[i] = w;
	}
	vkUpdateDescriptorSets(device, m_frame_count, writes, 0, nullptr);
}

Renderer::Renderer(uint32_t frameCount, bool validate, bool useRenderDoc) :
	m_frame_count(frameCount),
	m_validate(validate),
	m_use_render_doc(useRenderDoc),
	m_window(createWindow()),
	m_instance(createInstance()),
	m_debug_messenger(createDebugMessenger()),
	m_surface(createSurface()),
	device(createDevice()),
	format_depth(getFormatDepth()),
	m_supported_sample_counts(getSupportedSampleCounts()),
	m_pipeline_cache(VK_NULL_HANDLE),
	allocator(createAllocator()),
	m_queue(device.getQueue(m_queue_family_graphics, 0)),
	m_swapchain(createSwapchain()),
	m_swapchain_images(device.getSwapchainImages(m_swapchain)),
	m_swapchain_image_views(createSwapchainImageViews()),
	m_command_pool(device.createCommandPool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		m_queue_family_graphics)),
	m_transfer_command_pool(device.createCommandPool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		m_queue_family_graphics)),
	m_descriptor_set_layout_0(
		(device.allocateCommandBuffers(m_transfer_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, m_transfer_cmd.ptr()),
		createDescriptorSetLayout0())
		),
	m_descriptor_set_layout_dynamic(createDescriptorSetLayoutDynamic()),
	m_pipeline_layout_descriptor_set(createPipelineLayoutDescriptorSet()),

	m_fwd_p2_module(loadShaderModule(VK_SHADER_STAGE_VERTEX_BIT, "sha/fwd_p2")),
	m_screen_vertex_buffer(createScreenVertexBuffer()),

	m_sample_count(fitSampleCount(VK_SAMPLE_COUNT_64_BIT)),
	m_opaque_pass(createOpaquePass()),
	m_illum_technique(IllumTechnique::Ssgi),
	m_illum_technique_props(getIllumTechniqueProps()),
	m_color_resolve_pass(createColorResolvePass()),
	m_color_resolve_set_layout(createColorResolveSetLayout()),
	m_color_resolve_pipeline(createColorResolvePipeline()),
	m_depth_resolve_pass(createDepthResolvePass()),
	m_depth_resolve_set_layout(createDepthResolveSetLayout()),
	m_depth_resolve_pipeline(createDepthResolvePipeline()),
	m_depth_acc_pipeline(createDepthAccPipeline()),
	m_illumination_pass(createIlluminationPass()),
	m_illumination_set_layout(createIlluminationSetLayout()),
	m_illumination_pipeline(createIlluminationPipeline()),
	m_wsi_pass(createWsiPass()),
	m_wsi_set_layout(createWsiSetLayout()),
	m_wsi_pipeline(createWsiPipeline()),

	m_sampler_fb(createSamplerFb()),
	m_sampler_fb_mip(createSamplerFbMip()),
	m_sampler_fb_lin(createSamplerFbLin()),

	m_descriptor_pool(createDescriptorPool()),
	m_descriptor_pool_mip(createDescriptorPoolMip()),

	m_frames(createFrames()),
	m_pipeline_pool(4),
	m_model_pool(4),
	m_rnd(std::time(nullptr))
{
	std::memset(m_keys, 0, sizeof(m_keys));
	std::memset(m_keys_prev, 0, sizeof(m_keys_prev));

	bindFrameDescriptors();
}

Renderer::~Renderer(void)
{
	m_queue.waitIdle();

	m_model_pool.destroy(allocator);
	m_pipeline_pool.destroy(device);

	for (auto &f : m_frames)
		f.destroy(true);

	device.destroy(m_sampler_fb_lin);
	device.destroy(m_sampler_fb_mip);
	device.destroy(m_sampler_fb);

	m_wsi_pipeline.destroy(device);
	device.destroy(m_wsi_set_layout);
	device.destroy(m_wsi_pass);
	m_illumination_pipeline.destroy(device);
	device.destroy(m_illumination_set_layout);
	device.destroy(m_illumination_pass);
	m_depth_acc_pipeline.destroy(device);
	m_depth_resolve_pipeline.destroy(device);
	device.destroy(m_depth_resolve_set_layout);
	device.destroy(m_depth_resolve_pass);

	m_color_resolve_pipeline.destroy(device);
	device.destroy(m_color_resolve_set_layout);
	device.destroy(m_color_resolve_pass);

	device.destroy(m_opaque_pass);

	allocator.destroy(m_screen_vertex_buffer);
	device.destroy(m_fwd_p2_module);

	device.destroy(m_descriptor_pool_mip);
	device.destroy(m_descriptor_pool);
	device.destroy(m_pipeline_layout_descriptor_set);
	device.destroy(m_descriptor_set_layout_dynamic);
	device.destroy(m_descriptor_set_layout_0);

	device.destroy(m_transfer_command_pool);
	device.destroy(m_command_pool);
	for (auto &v : m_swapchain_image_views)
		device.destroy(v);
	device.destroy(m_swapchain);
	allocator.destroy();
	device.destroy();
	m_instance.destroy(m_surface);
	if (m_debug_messenger)
		m_instance.getProcAddr<PFN_vkDestroyDebugUtilsMessengerEXT>("vkDestroyDebugUtilsMessengerEXT")(m_instance, m_debug_messenger, nullptr);
	m_instance.destroy();
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Renderer::bindFrameDescriptors(void)
{
	static constexpr uint32_t const_img_writes_per_frame =
		1;	// wsi: output
	uint32_t img_writes_per_frame =
		const_img_writes_per_frame +
		m_illum_technique_props.descriptorCombinedImageSamplerCount;	// illum
	uint32_t img_writes_offset = 0;
	static constexpr uint32_t buf_writes_per_frame =
		1;	// illum: buffer
	uint32_t buf_writes_offset = img_writes_per_frame;
	uint32_t img_mip_writes_per_frame = m_illum_technique == IllumTechnique::Ssgi ?
		m_swapchain_mip_levels - 1 : 0;	// depth_acc
	uint32_t img_mip_writes_offset = img_writes_per_frame + buf_writes_per_frame;
	uint32_t writes_per_frame = img_writes_per_frame + buf_writes_per_frame + img_mip_writes_per_frame;

	VkDescriptorImageInfo image_infos[m_frame_count * (img_writes_per_frame + img_mip_writes_per_frame)];
	VkDescriptorBufferInfo buffer_infos[m_frame_count * buf_writes_per_frame];
	VkWriteDescriptorSet writes[m_frame_count * writes_per_frame];
	for (uint32_t i = 0; i < m_frame_count; i++) {
		auto &cur_frame = m_frames[i];
		auto &next_frame = m_frames[(i + 1) % m_frame_count];

		struct WriteImgDesc {
			VkDescriptorSet descriptorSet;
			uint32_t binding;
			VkSampler sampler;
			VkImageView imageView;
			VkImageLayout imageLayout;
		} write_img_descs[img_writes_per_frame];
		size_t write_img_descs_offset = 0;

		{
			WriteImgDesc descs[const_img_writes_per_frame] {
				{cur_frame.m_wsi_set, 0, m_sampler_fb, cur_frame.m_output_view, Vk::ImageLayout::ShaderReadOnlyOptimal}
			};
			for (size_t i = 0; i < array_size(descs); i++)
				write_img_descs[write_img_descs_offset++] = descs[i];
		}

		if (m_illum_technique == IllumTechnique::Potato) {
			WriteImgDesc descs[IllumTechnique::Data::Potato::descriptorCombinedImageSamplerCount] {
				{cur_frame.m_illumination_set, 1, m_sampler_fb_lin, cur_frame.m_cdepth_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
				{cur_frame.m_illumination_set, 2, m_sampler_fb_lin, cur_frame.m_albedo_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
				{cur_frame.m_illumination_set, 3, m_sampler_fb, cur_frame.m_normal_view, Vk::ImageLayout::ShaderReadOnlyOptimal}
			};
			for (size_t i = 0; i < array_size(descs); i++)
				write_img_descs[write_img_descs_offset++] = descs[i];
		}
		if (m_illum_technique == IllumTechnique::Ssgi) {
			if (m_sample_count == VK_SAMPLE_COUNT_1_BIT) {
				WriteImgDesc descs[IllumTechnique::Data::Ssgi::descriptorCombinedImageSamplerCount] {
					{cur_frame.m_illum_ssgi_fbs.m_depth_resolve_set, 0, m_sampler_fb, cur_frame.m_cdepth_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{cur_frame.m_illumination_set, 1, m_sampler_fb_mip, cur_frame.m_illum_ssgi_fbs.m_depth_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{cur_frame.m_illumination_set, 2, m_sampler_fb_lin, cur_frame.m_albedo_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{cur_frame.m_illumination_set, 3, m_sampler_fb, cur_frame.m_normal_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 4, m_sampler_fb_lin, cur_frame.m_illum_ssgi_fbs.m_depth_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 5, m_sampler_fb_lin, cur_frame.m_albedo_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 6, m_sampler_fb, cur_frame.m_normal_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 7, m_sampler_fb, cur_frame.m_illum_ssgi_fbs.m_step_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 8, m_sampler_fb, cur_frame.m_illum_ssgi_fbs.m_acc_path_pos_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 9, m_sampler_fb_lin, cur_frame.m_illum_ssgi_fbs.m_direct_light_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 10, m_sampler_fb, cur_frame.m_illum_ssgi_fbs.m_path_albedo_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 11, m_sampler_fb, cur_frame.m_illum_ssgi_fbs.m_path_direct_light_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 12, m_sampler_fb, cur_frame.m_illum_ssgi_fbs.m_path_incidence_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 13, m_sampler_fb_lin, cur_frame.m_output_view, Vk::ImageLayout::ShaderReadOnlyOptimal}
				};
				for (size_t i = 0; i < array_size(descs); i++)
					write_img_descs[write_img_descs_offset++] = descs[i];
			} else {
				WriteImgDesc descs[IllumTechnique::Data::Ssgi::msDescriptorCombinedImageSamplerCount] {
					{cur_frame.m_illum_ssgi_fbs.m_color_resolve_set, 0, m_sampler_fb, cur_frame.m_cdepth_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{cur_frame.m_illum_ssgi_fbs.m_color_resolve_set, 1, m_sampler_fb, cur_frame.m_albedo_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{cur_frame.m_illum_ssgi_fbs.m_color_resolve_set, 2, m_sampler_fb, cur_frame.m_normal_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{cur_frame.m_illum_ssgi_fbs.m_depth_resolve_set, 0, m_sampler_fb, cur_frame.m_cdepth_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{cur_frame.m_illumination_set, 1, m_sampler_fb, cur_frame.m_cdepth_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{cur_frame.m_illumination_set, 2, m_sampler_fb_mip, cur_frame.m_illum_ssgi_fbs.m_depth_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{cur_frame.m_illumination_set, 3, m_sampler_fb_lin, cur_frame.m_albedo_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{cur_frame.m_illumination_set, 4, m_sampler_fb, cur_frame.m_normal_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{cur_frame.m_illumination_set, 5, m_sampler_fb_lin, cur_frame.m_illum_ssgi_fbs.m_albedo_resolved_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{cur_frame.m_illumination_set, 6, m_sampler_fb, cur_frame.m_illum_ssgi_fbs.m_normal_resolved_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 7, m_sampler_fb_lin, cur_frame.m_illum_ssgi_fbs.m_depth_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 8, m_sampler_fb_lin, cur_frame.m_illum_ssgi_fbs.m_albedo_resolved_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 9, m_sampler_fb, cur_frame.m_illum_ssgi_fbs.m_normal_resolved_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 10, m_sampler_fb, cur_frame.m_illum_ssgi_fbs.m_step_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 11, m_sampler_fb, cur_frame.m_illum_ssgi_fbs.m_acc_path_pos_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 12, m_sampler_fb_lin, cur_frame.m_illum_ssgi_fbs.m_direct_light_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 13, m_sampler_fb, cur_frame.m_illum_ssgi_fbs.m_path_albedo_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 14, m_sampler_fb, cur_frame.m_illum_ssgi_fbs.m_path_direct_light_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 15, m_sampler_fb, cur_frame.m_illum_ssgi_fbs.m_path_incidence_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
					{next_frame.m_illumination_set, 16, m_sampler_fb_lin, cur_frame.m_output_view, Vk::ImageLayout::ShaderReadOnlyOptimal}
				};
				for (size_t i = 0; i < array_size(descs); i++)
					write_img_descs[write_img_descs_offset++] = descs[i];
			}
		}

		for (uint32_t j = 0; j < img_writes_per_frame; j++) {
			auto &ii = image_infos[i * (img_writes_per_frame + img_mip_writes_per_frame) + j];
			ii.sampler = write_img_descs[j].sampler;
			ii.imageView = write_img_descs[j].imageView;
			ii.imageLayout = write_img_descs[j].imageLayout;

			VkWriteDescriptorSet w{};
			w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			w.dstSet = write_img_descs[j].descriptorSet;
			w.dstBinding = write_img_descs[j].binding;
			w.dstArrayElement = 0;
			w.descriptorCount = 1;
			w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			w.pImageInfo = &ii;
			writes[i * writes_per_frame + img_writes_offset + j] = w;
		}

		struct WriteBufDesc {
			VkDescriptorSet descriptorSet;
			uint32_t binding;
			VkBuffer buffer;
		} write_buf_descs[buf_writes_per_frame] {
			{cur_frame.m_illumination_set, 0, cur_frame.m_illumination_buffer}
		};

		for (uint32_t j = 0; j < buf_writes_per_frame; j++) {
			auto &bi = buffer_infos[i * buf_writes_per_frame + j];
			bi.buffer = write_buf_descs[j].buffer;
			bi.offset = 0;
			bi.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet w{};
			w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			w.dstSet = write_buf_descs[j].descriptorSet;
			w.dstBinding = write_buf_descs[j].binding;
			w.dstArrayElement = 0;
			w.descriptorCount = 1;
			w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			w.pBufferInfo = &bi;
			writes[i * writes_per_frame + buf_writes_offset + j] = w;
		}

		if (m_illum_technique == IllumTechnique::Ssgi) {
			WriteImgDesc write_img_mip_descs[img_mip_writes_per_frame];
			for (uint32_t j = 0; j < img_mip_writes_per_frame; j++)
				write_img_mip_descs[j] = WriteImgDesc{cur_frame.m_illum_ssgi_fbs.m_depth_acc_sets[j], 0, m_sampler_fb,
					j == 0 ? cur_frame.m_illum_ssgi_fbs.m_depth_first_mip_view : cur_frame.m_illum_ssgi_fbs.m_depth_acc_views[j - 1], Vk::ImageLayout::ShaderReadOnlyOptimal};
			for (uint32_t j = 0; j < img_mip_writes_per_frame; j++) {
				auto &ii = image_infos[i * (img_writes_per_frame + img_mip_writes_per_frame) + img_writes_per_frame + j];
				ii.sampler = write_img_mip_descs[j].sampler;
				ii.imageView = write_img_mip_descs[j].imageView;
				ii.imageLayout = write_img_mip_descs[j].imageLayout;

				VkWriteDescriptorSet w{};
				w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				w.dstSet = write_img_mip_descs[j].descriptorSet;
				w.dstBinding = write_img_mip_descs[j].binding;
				w.dstArrayElement = 0;
				w.descriptorCount = 1;
				w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				w.pImageInfo = &ii;
				writes[i * writes_per_frame + img_mip_writes_offset + j] = w;
			}
		}
	}
	//for (size_t i = 0; i < m_frame_count * writes_per_frame; i++)
		//if (writes[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			//std::cout << "got: " << i << ", " << writes[i].dstBinding << ", " << writes[i].dstSet << std::endl;
	vkUpdateDescriptorSets(device, m_frame_count * writes_per_frame, writes, 0, nullptr);

	m_transfer_cmd.beginPrimary(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	{
		static constexpr uint32_t const_barrs_per_frame = 3;
		uint32_t barrs_per_frame = const_barrs_per_frame + m_illum_technique_props.barrsPerFrame;
		{
			VkImageMemoryBarrier img_barrs[barrs_per_frame * m_frame_count];
			for (uint32_t i = 0; i < m_frame_count; i++) {
				VkImage images[barrs_per_frame];
				size_t images_offset = 0;

				{
					VkImage imgs[const_barrs_per_frame] {
						m_frames[i].m_albedo,
						m_frames[i].m_normal,
						m_frames[i].m_output
					};
					for (uint32_t i = 0; i < array_size(imgs); i++)
						images[images_offset++] = imgs[i];
				}

				if (m_illum_technique == IllumTechnique::Ssgi) {
					if (m_sample_count == VK_SAMPLE_COUNT_1_BIT) {
						VkImage imgs[IllumTechnique::Data::Ssgi::barrsPerFrame] {
							m_frames[i].m_illum_ssgi_fbs.m_depth,
							m_frames[i].m_illum_ssgi_fbs.m_step,
							m_frames[i].m_illum_ssgi_fbs.m_acc_path_pos,
							m_frames[i].m_illum_ssgi_fbs.m_direct_light
						};
						for (uint32_t i = 0; i < array_size(imgs); i++)
							images[images_offset++] = imgs[i];
					} else {
						VkImage imgs[IllumTechnique::Data::Ssgi::msBarrsPerFrame] {
							m_frames[i].m_illum_ssgi_fbs.m_depth,
							m_frames[i].m_illum_ssgi_fbs.m_albedo_resolved,
							m_frames[i].m_illum_ssgi_fbs.m_normal_resolved,
							m_frames[i].m_illum_ssgi_fbs.m_step,
							m_frames[i].m_illum_ssgi_fbs.m_acc_path_pos,
							m_frames[i].m_illum_ssgi_fbs.m_direct_light
						};
						for (uint32_t i = 0; i < array_size(imgs); i++)
							images[images_offset++] = imgs[i];
					}
				}
				for (uint32_t j = 0; j < barrs_per_frame; j++)
					img_barrs[i * barrs_per_frame + j] = VkImageMemoryBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr,
						Vk::Access::TransferWriteBit, Vk::Access::TransferReadBit,
						Vk::ImageLayout::Undefined, Vk::ImageLayout::TransferDstOptimal, 0, 0, images[j],
						{Vk::ImageAspect::ColorBit, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS}};
			}
			m_transfer_cmd.pipelineBarrier(Vk::PipelineStage::TransferBit, Vk::PipelineStage::TransferBit,
				0, 0, nullptr, 0, nullptr, barrs_per_frame * m_frame_count, img_barrs);
		}

		{
			VkClearColorValue cv_f32_zero;
			VkClearColorValue cv_i32_zero;
			for (size_t i = 0; i < 4; i++) {
				cv_f32_zero.float32[i] = 0.0f;
				cv_i32_zero.int32[i] = 0;
			}
			for (uint32_t i = 0; i < m_frame_count; i++) {
				struct ImgDesc {
					VkImage img;
					const VkClearColorValue *pClear;
				} images[barrs_per_frame];
				size_t images_offset = 0;

				{
					ImgDesc imgs[const_barrs_per_frame] {
						{m_frames[i].m_albedo, &cv_f32_zero},
						{m_frames[i].m_normal, &cv_f32_zero},
						{m_frames[i].m_output, &cv_f32_zero}
					};
					for (uint32_t i = 0; i < array_size(imgs); i++)
						images[images_offset++] = imgs[i];
				}

				if (m_illum_technique == IllumTechnique::Ssgi) {
					if (m_sample_count == VK_SAMPLE_COUNT_1_BIT) {
						ImgDesc imgs[IllumTechnique::Data::Ssgi::barrsPerFrame] {
							{m_frames[i].m_illum_ssgi_fbs.m_depth, &cv_f32_zero},
							{m_frames[i].m_illum_ssgi_fbs.m_step, &cv_i32_zero},
							{m_frames[i].m_illum_ssgi_fbs.m_acc_path_pos, &cv_i32_zero},
							{m_frames[i].m_illum_ssgi_fbs.m_direct_light, &cv_f32_zero},
						};
						for (uint32_t i = 0; i < array_size(imgs); i++)
							images[images_offset++] = imgs[i];
					} else {
						ImgDesc imgs[IllumTechnique::Data::Ssgi::msBarrsPerFrame] {
							{m_frames[i].m_illum_ssgi_fbs.m_depth, &cv_f32_zero},
							{m_frames[i].m_illum_ssgi_fbs.m_albedo_resolved, &cv_f32_zero},
							{m_frames[i].m_illum_ssgi_fbs.m_normal_resolved, &cv_f32_zero},
							{m_frames[i].m_illum_ssgi_fbs.m_step, &cv_i32_zero},
							{m_frames[i].m_illum_ssgi_fbs.m_acc_path_pos, &cv_i32_zero},
							{m_frames[i].m_illum_ssgi_fbs.m_direct_light, &cv_f32_zero},
						};
						for (uint32_t i = 0; i < array_size(imgs); i++)
							images[images_offset++] = imgs[i];
					}
				}

				for (uint32_t j = 0; j < barrs_per_frame; j++) {
					VkImageSubresourceRange range{Vk::ImageAspect::ColorBit, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS};
					m_transfer_cmd.clearColorImage(images[j].img, Vk::ImageLayout::TransferDstOptimal, images[j].pClear, 1, &range);
				}
			}
		}

		{
			uint32_t tbarrs_per_frame = const_barrs_per_frame + m_illum_technique_props.addBarrsPerFrame;
			uint32_t img_barr_count = tbarrs_per_frame * m_frame_count;
			VkImageMemoryBarrier img_barrs[img_barr_count];
			for (uint32_t i = 0; i < m_frame_count; i++) {
				struct ImgDesc {
					VkImage img;
					VkImageLayout imageLayout;
				} images[tbarrs_per_frame];
				size_t images_offset = 0;

				{
					ImgDesc imgs[const_barrs_per_frame] {
						{m_frames[i].m_albedo, Vk::ImageLayout::TransferDstOptimal},
						{m_frames[i].m_normal, Vk::ImageLayout::TransferDstOptimal},
						{m_frames[i].m_output, Vk::ImageLayout::TransferDstOptimal},
					};
					for (uint32_t i = 0; i < array_size(imgs); i++)
						images[images_offset++] = imgs[i];
				}

				if (m_illum_technique == IllumTechnique::Ssgi) {
					if (m_sample_count == VK_SAMPLE_COUNT_1_BIT) {
						ImgDesc imgs[IllumTechnique::Data::Ssgi::addBarrsPerFrame] {
							{m_frames[i].m_illum_ssgi_fbs.m_depth, Vk::ImageLayout::TransferDstOptimal},
							{m_frames[i].m_illum_ssgi_fbs.m_step, Vk::ImageLayout::TransferDstOptimal},
							{m_frames[i].m_illum_ssgi_fbs.m_acc_path_pos, Vk::ImageLayout::TransferDstOptimal},
							{m_frames[i].m_illum_ssgi_fbs.m_direct_light, Vk::ImageLayout::TransferDstOptimal},
							{m_frames[i].m_illum_ssgi_fbs.m_path_albedo, Vk::ImageLayout::Undefined},
							{m_frames[i].m_illum_ssgi_fbs.m_path_direct_light, Vk::ImageLayout::Undefined},
							{m_frames[i].m_illum_ssgi_fbs.m_path_incidence, Vk::ImageLayout::Undefined}
						};
						for (uint32_t i = 0; i < array_size(imgs); i++)
							images[images_offset++] = imgs[i];
					} else {
						ImgDesc imgs[IllumTechnique::Data::Ssgi::msAddBarrsPerFrame] {
							{m_frames[i].m_illum_ssgi_fbs.m_depth, Vk::ImageLayout::TransferDstOptimal},
							{m_frames[i].m_illum_ssgi_fbs.m_albedo_resolved, Vk::ImageLayout::TransferDstOptimal},
							{m_frames[i].m_illum_ssgi_fbs.m_normal_resolved, Vk::ImageLayout::TransferDstOptimal},
							{m_frames[i].m_illum_ssgi_fbs.m_step, Vk::ImageLayout::TransferDstOptimal},
							{m_frames[i].m_illum_ssgi_fbs.m_acc_path_pos, Vk::ImageLayout::TransferDstOptimal},
							{m_frames[i].m_illum_ssgi_fbs.m_direct_light, Vk::ImageLayout::TransferDstOptimal},
							{m_frames[i].m_illum_ssgi_fbs.m_path_albedo, Vk::ImageLayout::Undefined},
							{m_frames[i].m_illum_ssgi_fbs.m_path_direct_light, Vk::ImageLayout::Undefined},
							{m_frames[i].m_illum_ssgi_fbs.m_path_incidence, Vk::ImageLayout::Undefined}
						};
						for (uint32_t i = 0; i < array_size(imgs); i++)
							images[images_offset++] = imgs[i];
					}
				}

				for (uint32_t j = 0; j < tbarrs_per_frame; j++)
					img_barrs[i * tbarrs_per_frame + j] = VkImageMemoryBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr,
						Vk::Access::TransferWriteBit, Vk::Access::TransferReadBit,
						images[j].imageLayout,
						Vk::ImageLayout::ShaderReadOnlyOptimal, 0, 0, images[j].img,
						{Vk::ImageAspect::ColorBit, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS}};
			}
			m_transfer_cmd.pipelineBarrier(Vk::PipelineStage::TransferBit, Vk::PipelineStage::TransferBit,
				0, 0, nullptr, 0, nullptr, img_barr_count, img_barrs);
		}
	}
	m_transfer_cmd.end();
	VkSubmitInfo submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = m_transfer_cmd.ptr();
	m_queue.submit(1, &submit, VK_NULL_HANDLE);
	m_queue.waitIdle();
}

void Renderer::recreateSwapchain(void)
{
	m_queue.waitIdle();
	for (auto &f : m_frames)
		f.destroy();
	device.destroy(m_descriptor_pool_mip);
	size_t frame_count = m_frames.size();
	uint8_t frames[frame_count * sizeof(Frame)];
	std::memcpy(frames, m_frames.data(), frame_count * sizeof(Frame));
	m_frames.clear();
	for (auto &i : m_swapchain_image_views)
		device.destroy(i);
	device.destroy(m_swapchain);

	m_swapchain = createSwapchain();
	m_swapchain_images = device.getSwapchainImages(m_swapchain);
	m_swapchain_image_views = createSwapchainImageViews();

	m_descriptor_pool_mip = createDescriptorPoolMip();
	size_t sets_mip_stride = (m_swapchain_mip_levels - 1) * sets_per_frame_mip;
	size_t sets_mip_size = m_frame_count * sets_mip_stride;
	VkDescriptorSet sets_mip[sets_mip_size];
	VkDescriptorSetLayout set_layouts_mip[sets_mip_size];
	for (size_t i = 0; i < sets_mip_size; i++)
		set_layouts_mip[i] = m_depth_resolve_set_layout;
	device.allocateDescriptorSets(m_descriptor_pool_mip, sets_mip_size, set_layouts_mip, sets_mip);

	for (size_t i = 0; i < frame_count; i++) {
		auto &f = reinterpret_cast<Frame*>(frames)[i];
		m_frames.emplace(*this, i, f.m_transfer_cmd, f.m_cmd,
			f.m_descriptor_set_0, f.m_descriptor_set_dynamic,
			f.m_illum_ssgi_fbs.m_color_resolve_set, f.m_illum_ssgi_fbs.m_depth_resolve_set,
			f.m_illumination_set, f.m_wsi_set,
			&sets_mip[sets_mip_stride],
			f.m_dyn_buffer);
	}
	bindFrameDescriptors();
}

size_t Renderer::m_keys_update[Renderer::key_update_count] {
	GLFW_KEY_F11,
	GLFW_KEY_W,
	GLFW_KEY_A,
	GLFW_KEY_S,
	GLFW_KEY_D,
	GLFW_KEY_LEFT_SHIFT,
	GLFW_KEY_ESCAPE,
	GLFW_KEY_N,
	GLFW_KEY_SPACE
};

void Renderer::pollEvents(void)
{
	glfwPollEvents();
	{
		std::memcpy(m_keys_prev, m_keys, sizeof(m_keys));
		for (size_t i = 0; i < key_update_count; i++) {
			auto glfw_key = m_keys_update[i];
			m_keys[glfw_key] = glfwGetKey(m_window, glfw_key);
		}
		glfwGetCursorPos(m_window, &m_cursor.x, &m_cursor.y);
		if (m_pending_cursor_mode != ~0ULL) {
			glfwSetInputMode(m_window, GLFW_CURSOR, m_pending_cursor_mode);
			m_pending_cursor_mode = ~0ULL;
		}
	}
	if (keyReleased(GLFW_KEY_F11)) {
		m_fullscreen = !m_fullscreen;
		int monitor_count;
		auto monitors = glfwGetMonitors(&monitor_count);
		if (monitors == nullptr)
			throwGlfwError();
		if (m_fullscreen) {
			glfwGetWindowPos(m_window, &m_window_last_pos.x, &m_window_last_pos.y);
			glfwGetWindowSize(m_window, &m_window_last_size.x, &m_window_last_size.y);
			if (monitor_count > 0) {
				auto mode = glfwGetVideoMode(monitors[0]);
				glfwSetWindowMonitor(m_window, monitors[0], 0, 0, mode->width, mode->height, mode->refreshRate);
			}
		} else
			glfwSetWindowMonitor(m_window, nullptr, m_window_last_pos.x, m_window_last_pos.y, m_window_last_size.x, m_window_last_size.y, GLFW_DONT_CARE);
	}
}

bool Renderer::shouldClose(void) const
{
	return glfwWindowShouldClose(m_window);
}

bool Renderer::keyState(int glfw_key) const
{
	return m_keys[glfw_key];
}

bool Renderer::keyPressed(int glfw_key) const
{
	return !m_keys_prev[glfw_key] && m_keys[glfw_key];
}

bool Renderer::keyReleased(int glfw_key) const
{
	return m_keys_prev[glfw_key] && !m_keys[glfw_key];
}

glm::dvec2 Renderer::cursor(void) const
{
	return m_cursor;
}

void Renderer::setCursorMode(bool show)
{
	m_pending_cursor_mode = show ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
}

void Renderer::resetFrame(void)
{
	m_frames[m_current_frame].reset();
}

void Renderer::render(Map &map, const Camera &camera)
{
	m_frames[m_current_frame].render(map, camera);
	m_current_frame = (m_current_frame + 1) % m_frame_count;
}

double Renderer::zrand(void)
{
	return static_cast<double>(m_rnd()) / static_cast<double>(std::numeric_limits<decltype(m_rnd())>::max());
}

Vk::BufferAllocation Renderer::Frame::createDynBufferStaging(void)
{
	VkBufferCreateInfo bci{};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.size = dyn_buffer_size;
	bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VmaAllocationCreateInfo aci{};
	aci.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	aci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	return m_r.allocator.createBuffer(bci, aci, &m_dyn_buffer_staging_ptr);
}

Vk::ImageView Renderer::Frame::createFbImage(VkFormat format, VkImageAspectFlags aspect, VkImageUsageFlags usage, Vk::ImageAllocation *pAllocation)
{
	VkImageCreateInfo ici{};
	ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ici.imageType = VK_IMAGE_TYPE_2D;
	ici.format = format;
	ici.extent = VkExtent3D{m_r.m_swapchain_extent.width, m_r.m_swapchain_extent.height, 1};
	ici.mipLevels = 1;
	ici.arrayLayers = 1;
	ici.samples = VK_SAMPLE_COUNT_1_BIT;
	ici.tiling = VK_IMAGE_TILING_OPTIMAL;
	ici.usage = usage;
	VmaAllocationCreateInfo aci{};
	aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	*pAllocation = m_r.allocator.createImage(ici, aci);
	return m_r.createImageView(*pAllocation, VK_IMAGE_VIEW_TYPE_2D, format, aspect);
}

Vk::ImageView Renderer::Frame::createFbImageMs(VkFormat format, VkImageAspectFlags aspect, VkImageUsageFlags usage, Vk::ImageAllocation *pAllocation)
{
	VkImageCreateInfo ici{};
	ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ici.imageType = VK_IMAGE_TYPE_2D;
	ici.format = format;
	ici.extent = VkExtent3D{m_r.m_swapchain_extent.width, m_r.m_swapchain_extent.height, 1};
	ici.mipLevels = 1;
	ici.arrayLayers = 1;
	ici.samples = m_r.m_sample_count;
	ici.tiling = VK_IMAGE_TILING_OPTIMAL;
	ici.usage = usage;
	VmaAllocationCreateInfo aci{};
	aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	*pAllocation = m_r.allocator.createImage(ici, aci);
	return m_r.createImageView(*pAllocation, VK_IMAGE_VIEW_TYPE_2D, format, aspect);
}

Vk::ImageView Renderer::Frame::createFbImageResolved(VkFormat format, VkImageAspectFlags aspect, VkImageUsageFlags usage, Vk::ImageAllocation *pAllocation)
{
	if (m_r.m_sample_count == VK_SAMPLE_COUNT_1_BIT) {
		*pAllocation = Vk::ImageAllocation(VK_NULL_HANDLE, VK_NULL_HANDLE);
		return VK_NULL_HANDLE;
	}
	VkImageCreateInfo ici{};
	ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ici.imageType = VK_IMAGE_TYPE_2D;
	ici.format = format;
	ici.extent = VkExtent3D{m_r.m_swapchain_extent.width, m_r.m_swapchain_extent.height, 1};
	ici.mipLevels = 1;
	ici.arrayLayers = 1;
	ici.samples = VK_SAMPLE_COUNT_1_BIT;
	ici.tiling = VK_IMAGE_TILING_OPTIMAL;
	ici.usage = usage;
	VmaAllocationCreateInfo aci{};
	aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	*pAllocation = m_r.allocator.createImage(ici, aci);
	return m_r.createImageView(*pAllocation, VK_IMAGE_VIEW_TYPE_2D, format, aspect);
}

Vk::ImageView Renderer::Frame::createFbImageMip(VkFormat format, VkImageAspectFlags aspect, VkImageUsageFlags usage, Vk::ImageAllocation *pAllocation)
{
	VkImageCreateInfo ici{};
	ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ici.imageType = VK_IMAGE_TYPE_2D;
	ici.format = format;
	ici.extent = VkExtent3D{m_r.m_swapchain_extent_mip.width, m_r.m_swapchain_extent_mip.height, 1};
	ici.mipLevels = m_r.m_swapchain_mip_levels;
	ici.arrayLayers = 1;
	ici.samples = VK_SAMPLE_COUNT_1_BIT;
	ici.tiling = VK_IMAGE_TILING_OPTIMAL;
	ici.usage = usage;
	VmaAllocationCreateInfo aci{};
	aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	*pAllocation = m_r.allocator.createImage(ici, aci);
	return m_r.createImageView(*pAllocation, VK_IMAGE_VIEW_TYPE_2D, format, aspect);
}

Renderer::IllumTechnique::Data::Ssgi::Fbs Renderer::Frame::createIllumSsgiFbs(VkDescriptorSet descriptorSetColorResolve, VkDescriptorSet descriptorSetDepthResolve,
	const VkDescriptorSet *pDescriptorSetsMip)
{
	IllumTechnique::Data::Ssgi::Fbs res;
	if (m_r.m_illum_technique != IllumTechnique::Ssgi)
		return res;

	if (m_r.m_sample_count > VK_SAMPLE_COUNT_1_BIT) {
		res.m_albedo_resolved_view = createFbImage(VK_FORMAT_R8G8B8A8_SRGB, Vk::ImageAspect::ColorBit,
			Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit | Vk::ImageUsage::TransferDst, &res.m_albedo_resolved);
		res.m_normal_resolved_view = createFbImage(VK_FORMAT_R16G16B16A16_SFLOAT, Vk::ImageAspect::ColorBit,
			Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit | Vk::ImageUsage::TransferDst, &res.m_normal_resolved);

		res.m_color_resolve_fb = res.createColorResolveFb(m_r);
		res.m_color_resolve_set = descriptorSetColorResolve;
	}

	res.m_depth_view = createFbImageMip(VK_FORMAT_R32_SFLOAT, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit | Vk::ImageUsage::TransferDst, &res.m_depth);
	res.m_depth_first_mip_view = m_r.createImageViewMip(res.m_depth, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32_SFLOAT, Vk::ImageAspect::ColorBit,
		0, 1);

	res.m_depth_resolve_fb = res.createDepthResolveFb(m_r);
	res.m_depth_resolve_set = descriptorSetDepthResolve;
	res.m_depth_acc_views = res.createDepthAccViews(m_r);
	res.m_depth_acc_fbs = res.createDepthAccFbs(m_r);
	res.m_depth_acc_sets = res.createDepthAccSets(m_r, pDescriptorSetsMip);

	res.m_step_view = createFbImage(VK_FORMAT_R8_SINT, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit | Vk::ImageUsage::TransferDst, &res.m_step);
	res.m_acc_path_pos_view = createFbImage(VK_FORMAT_R16G16B16A16_UINT, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit | Vk::ImageUsage::TransferDst, &res.m_acc_path_pos);
	res.m_direct_light_view = createFbImage(VK_FORMAT_R16G16B16A16_SFLOAT, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit | Vk::ImageUsage::TransferDst, &res.m_direct_light);
	res.m_path_albedo_view = createFbImage(VK_FORMAT_R16G16B16A16_SFLOAT, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit, &res.m_path_albedo);
	res.m_path_direct_light_view = createFbImage(VK_FORMAT_R16G16B16A16_SFLOAT, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit, &res.m_path_direct_light);
	res.m_path_incidence_view = createFbImage(VK_FORMAT_R16G16B16A16_SFLOAT, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit, &res.m_path_incidence);
	return res;
}

Vk::Framebuffer Renderer::IllumTechnique::Data::Ssgi::Fbs::createColorResolveFb(Renderer &r)
{
	VkFramebufferCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	ci.renderPass = r.m_color_resolve_pass;
	VkImageView atts[] {
		m_albedo_resolved_view,
		m_normal_resolved_view
	};
	ci.attachmentCount = array_size(atts);
	ci.pAttachments = atts;
	ci.width = r.m_swapchain_extent.width;
	ci.height = r.m_swapchain_extent.height;
	ci.layers = 1;
	return r.device.createFramebuffer(ci);
}

Vk::Framebuffer Renderer::IllumTechnique::Data::Ssgi::Fbs::createDepthResolveFb(Renderer &r)
{
	VkFramebufferCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	ci.renderPass = r.m_depth_resolve_pass;
	VkImageView atts[] {
		m_depth_first_mip_view
	};
	ci.attachmentCount = array_size(atts);
	ci.pAttachments = atts;
	ci.width = r.m_swapchain_extent_mip.width;
	ci.height = r.m_swapchain_extent_mip.height;
	ci.layers = 1;
	return r.device.createFramebuffer(ci);
}

vector<Vk::ImageView> Renderer::IllumTechnique::Data::Ssgi::Fbs::createDepthAccViews(Renderer &r)
{
	size_t acc_sets = r.m_swapchain_mip_levels - 1;
	auto res = vector<Vk::ImageView>(acc_sets);
	for (size_t i = 0; i < acc_sets; i++)
		res[i] = r.createImageViewMip(m_depth, VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32_SFLOAT, Vk::ImageAspect::ColorBit, i + 1, 1);
	return res;
}

vector<Vk::Framebuffer> Renderer::IllumTechnique::Data::Ssgi::Fbs::createDepthAccFbs(Renderer &r)
{
	size_t acc_sets = r.m_swapchain_mip_levels - 1;
	auto res = vector<Vk::Framebuffer>(acc_sets);
	for (size_t i = 0; i < acc_sets; i++) {
		VkFramebufferCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		ci.renderPass = r.m_depth_resolve_pass;
		VkImageView atts[] {
			m_depth_acc_views[i]
		};
		ci.attachmentCount = array_size(atts);
		ci.pAttachments = atts;
		auto &cext = r.m_swapchain_extent_mips[i + 1];
		ci.width = cext.width;
		ci.height = cext.height;
		ci.layers = 1;
		res[i] = r.device.createFramebuffer(ci);
	}
	return res;
}

vector<VkDescriptorSet> Renderer::IllumTechnique::Data::Ssgi::Fbs::createDepthAccSets(Renderer &r, const VkDescriptorSet *pDescriptorSetsMip)
{
	size_t acc_sets = r.m_swapchain_mip_levels - 1;
	auto res = vector<VkDescriptorSet>(acc_sets);
	std::memcpy(res.data(), pDescriptorSetsMip, acc_sets * sizeof(VkDescriptorSet));
	return res;
}

void Renderer::IllumTechnique::Data::Ssgi::Fbs::destroy(Renderer &r)
{
	r.device.destroy(m_depth_resolve_fb);
	for (auto v : m_depth_acc_views)
		r.device.destroy(v);
	for (auto f : m_depth_acc_fbs)
		r.device.destroy(f);

	r.device.destroy(m_path_incidence_view);
	r.allocator.destroy(m_path_incidence);
	r.device.destroy(m_path_direct_light_view);
	r.allocator.destroy(m_path_direct_light);
	r.device.destroy(m_path_albedo_view);
	r.allocator.destroy(m_path_albedo);
	r.device.destroy(m_direct_light_view);
	r.allocator.destroy(m_direct_light);
	r.device.destroy(m_acc_path_pos_view);
	r.allocator.destroy(m_acc_path_pos);
	r.device.destroy(m_step_view);
	r.allocator.destroy(m_step);

	r.device.destroy(m_depth_first_mip_view);
	r.device.destroy(m_depth_view);
	r.allocator.destroy(m_depth);

	if (r.m_sample_count > VK_SAMPLE_COUNT_1_BIT) {
		r.device.destroy(m_color_resolve_fb);

		r.device.destroy(m_normal_resolved_view);
		r.allocator.destroy(m_normal_resolved);
		r.device.destroy(m_albedo_resolved_view);
		r.allocator.destroy(m_albedo_resolved);
	}
}

Vk::Framebuffer Renderer::Frame::createOpaqueFb(void)
{
	VkFramebufferCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	ci.renderPass = m_r.m_opaque_pass;

	VkImageView atts[] {
		m_depth_buffer_view,
		m_cdepth_view,
		m_albedo_view,
		m_normal_view
	};
	ci.attachmentCount = array_size(atts);
	ci.pAttachments = atts;
	ci.width = m_r.m_swapchain_extent.width;
	ci.height = m_r.m_swapchain_extent.height;
	ci.layers = 1;
	return m_r.device.createFramebuffer(ci);
}

Vk::Framebuffer Renderer::Frame::createIlluminationFb(void)
{
	VkFramebufferCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

	if (m_r.m_illum_technique == IllumTechnique::Potato) {
		ci.renderPass = m_r.m_illumination_pass;
		VkImageView atts[] {
			m_output_view
		};
		ci.attachmentCount = array_size(atts);
		ci.pAttachments = atts;
		ci.width = m_r.m_swapchain_extent.width;
		ci.height = m_r.m_swapchain_extent.height;
		ci.layers = 1;
		return m_r.device.createFramebuffer(ci);
	}
	if (m_r.m_illum_technique == IllumTechnique::Ssgi) {
		ci.renderPass = m_r.m_illumination_pass;
		VkImageView atts[] {
			m_illum_ssgi_fbs.m_step_view,
			m_illum_ssgi_fbs.m_acc_path_pos_view,
			m_illum_ssgi_fbs.m_direct_light_view,
			m_illum_ssgi_fbs.m_path_albedo_view,
			m_illum_ssgi_fbs.m_path_direct_light_view,
			m_illum_ssgi_fbs.m_path_incidence_view,
			m_output_view
		};
		ci.attachmentCount = array_size(atts);
		ci.pAttachments = atts;
		ci.width = m_r.m_swapchain_extent.width;
		ci.height = m_r.m_swapchain_extent.height;
		ci.layers = 1;
		return m_r.device.createFramebuffer(ci);
	}
	throw std::runtime_error("createIlluminationFb");
}

Vk::BufferAllocation Renderer::Frame::createIlluminationBuffer(void)
{
	VkBufferCreateInfo bci{};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.size = sizeof(Illumination);
	bci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	VmaAllocationCreateInfo aci{};
	aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	return m_r.allocator.createBuffer(bci, aci);
}

Vk::Framebuffer Renderer::Frame::createWsiFb(void)
{
	VkFramebufferCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	ci.renderPass = m_r.m_wsi_pass;
	VkImageView atts[] {
		m_r.m_swapchain_image_views[m_i]
	};
	ci.attachmentCount = array_size(atts);
	ci.pAttachments = atts;
	ci.width = m_r.m_swapchain_extent.width;
	ci.height = m_r.m_swapchain_extent.height;
	ci.layers = 1;
	return m_r.device.createFramebuffer(ci);
}

Renderer::Frame::Frame(Renderer &r, size_t i, VkCommandBuffer transferCmd, VkCommandBuffer cmd,
	VkDescriptorSet descriptorSet0, VkDescriptorSet descriptorSetDynamic,
	VkDescriptorSet descriptorSetColorResolve, VkDescriptorSet descriptorSetDepthResolve,
	VkDescriptorSet descriptorSetIllum, VkDescriptorSet descriptorSetWsi,
	const VkDescriptorSet *pDescriptorSetsMip,
	Vk::BufferAllocation dynBuffer) :
	m_r(r),
	m_i(i),
	m_transfer_cmd(transferCmd),
	m_cmd(cmd),
	m_frame_done(r.device.createFence(0)),
	m_render_done(r.device.createSemaphore()),
	m_image_ready(r.device.createSemaphore()),
	m_descriptor_set_0(descriptorSet0),
	m_descriptor_set_dynamic(descriptorSetDynamic),
	m_dyn_buffer_staging(createDynBufferStaging()),
	m_dyn_buffer(dynBuffer),
	m_depth_buffer_view(createFbImageMs(m_r.format_depth, Vk::ImageAspect::DepthBit,
		Vk::ImageUsage::DepthStencilAttachmentBit | Vk::ImageUsage::SampledBit, &m_depth_buffer)),
	m_cdepth_view(createFbImageMs(VK_FORMAT_R32_SFLOAT, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit, &m_cdepth)),
	m_albedo_view(createFbImageMs(VK_FORMAT_R8G8B8A8_SRGB, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit | Vk::ImageUsage::TransferDst, &m_albedo)),
	m_normal_view(createFbImageMs(VK_FORMAT_R16G16B16A16_SFLOAT, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit | Vk::ImageUsage::TransferDst, &m_normal)),
	m_illum_ssgi_fbs(createIllumSsgiFbs(descriptorSetColorResolve, descriptorSetDepthResolve, pDescriptorSetsMip)),
	m_output_view(createFbImage(VK_FORMAT_R16G16B16A16_SFLOAT, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit | Vk::ImageUsage::TransferDst, &m_output)),
	m_opaque_fb(createOpaqueFb()),
	m_illumination_fb(createIlluminationFb()),
	m_illumination_set(descriptorSetIllum),
	m_illumination_buffer(createIlluminationBuffer()),
	m_wsi_fb(createWsiFb()),
	m_wsi_set(descriptorSetWsi)
{
}

void Renderer::Frame::destroy(bool with_ext_res)
{
	m_r.device.destroy(m_wsi_fb);
	m_r.allocator.destroy(m_illumination_buffer);
	m_r.device.destroy(m_illumination_fb);
	m_r.device.destroy(m_opaque_fb);

	if (m_r.m_illum_technique == IllumTechnique::Ssgi)
		m_illum_ssgi_fbs.destroy(m_r);
	m_r.device.destroy(m_output_view);
	m_r.allocator.destroy(m_output);

	m_r.device.destroy(m_normal_view);
	m_r.allocator.destroy(m_normal);
	m_r.device.destroy(m_albedo_view);
	m_r.allocator.destroy(m_albedo);
	m_r.device.destroy(m_cdepth_view);
	m_r.allocator.destroy(m_cdepth);
	m_r.device.destroy(m_depth_buffer_view);
	m_r.allocator.destroy(m_depth_buffer);

	if (with_ext_res)
		m_r.allocator.destroy(m_dyn_buffer);
	m_r.allocator.destroy(m_dyn_buffer_staging);

	m_r.device.destroy(m_frame_done);
	m_r.device.destroy(m_render_done);
	m_r.device.destroy(m_image_ready);
}

void Renderer::Frame::reset(void)
{
	if (m_ever_submitted) {
		m_r.device.wait(m_frame_done);
		m_r.device.reset(m_frame_done);
	}
}

void Renderer::Frame::render(Map &map, const Camera &camera)
{
	auto &sex = m_r.m_swapchain_extent;
	auto &sex_mip = m_r.m_swapchain_extent_mip;

	uint32_t swapchain_index;
	vkAssert(Vk::ext.vkAcquireNextImageKHR(m_r.device, m_r.m_swapchain, ~0ULL, m_image_ready, VK_NULL_HANDLE, &swapchain_index));

	m_dyn_buffer_size = 0;
	m_transfer_cmd.beginPrimary(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkClearColorValue cv_grey;
	cv_grey.float32[0] = 0.5f;
	cv_grey.float32[1] = 0.5f;
	cv_grey.float32[2] = 0.5f;
	cv_grey.float32[3] = 1.0f;
	VkClearDepthStencilValue cv_d0;
	cv_d0.depth = 0.0f;
	VkClearColorValue cv_zero;
	for (size_t i = 0; i < 4; i++)
		cv_zero.float32[i] = 0.0f;
	/*VkClearColorValue cv_one;
	for (size_t i = 0; i < 4; i++)
		cv_one.float32[i] = 1.0f;*/
	/*VkClearColorValue cv_r0g0zm1;
	for (size_t i = 0; i < 4; i++)
		cv_r0g0zm1.float32[i] = 0.0f;
	cv_r0g0zm1.float32[3] = -1.0f;*/

	{
		m_cmd.beginPrimary(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		m_cmd.setExtent(m_r.m_swapchain_extent);
		m_cmd.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_r.m_pipeline_layout_descriptor_set,
			0, 1, &m_descriptor_set_0, 0, nullptr);
		{
			VkRenderPassBeginInfo bi{};
			bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			bi.renderPass = m_r.m_opaque_pass;
			bi.framebuffer = m_opaque_fb;
			bi.renderArea = VkRect2D{{0, 0}, sex};

			VkClearValue cvs[] {
				{ .depthStencil = cv_d0, },
				{ .color = cv_zero },
				{ .color = cv_grey },
				{ .color = cv_zero }
			};
			bi.clearValueCount = array_size(cvs);
			bi.pClearValues = cvs;
			m_cmd.beginRenderPass(bi, VK_SUBPASS_CONTENTS_INLINE);
		}
		render_subset(map, OpaqueRender::id);
		m_cmd.endRenderPass();
		{
			m_r.allocator.flushAllocation(m_dyn_buffer_staging, 0, m_dyn_buffer_size);
			{
				VkBufferCopy region {0, 0, m_dyn_buffer_size};
				if (region.size > 0)
					m_transfer_cmd.copyBuffer(m_dyn_buffer_staging, m_dyn_buffer, 1, &region);
			}
		}

		{
			VkMemoryBarrier barrier { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
				Vk::Access::ColorAttachmentWriteBit | Vk::Access::DepthStencilAttachmentWriteBit, Vk::Access::ShaderReadBit };
			m_cmd.pipelineBarrier(Vk::PipelineStage::ColorAttachmentOutputBit | Vk::PipelineStage::LateFragmentTestsBit,
				Vk::PipelineStage::FragmentShaderBit, 0,
				1, &barrier, 0, nullptr, 0, nullptr);
		}

		if (m_r.m_illum_technique == IllumTechnique::Ssgi) {
			if (m_r.m_sample_count > VK_SAMPLE_COUNT_1_BIT) {
				{
					VkRenderPassBeginInfo bi{};
					bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					bi.renderPass = m_r.m_color_resolve_pass;
					bi.framebuffer = m_illum_ssgi_fbs.m_color_resolve_fb;
					bi.renderArea = VkRect2D{{0, 0}, sex};
					m_cmd.beginRenderPass(bi, VK_SUBPASS_CONTENTS_INLINE);
				}
				m_cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_r.m_color_resolve_pipeline);
				m_cmd.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_r.m_color_resolve_pipeline.pipelineLayout,
					0, 1, &m_illum_ssgi_fbs.m_color_resolve_set, 0, nullptr);
				m_cmd.bindVertexBuffer(0, m_r.m_screen_vertex_buffer, 0);
				m_cmd.draw(3, 1, 0, 0);
				m_cmd.endRenderPass();
			}

			m_cmd.setExtent(sex_mip);
			{
				VkRenderPassBeginInfo bi{};
				bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				bi.renderPass = m_r.m_depth_resolve_pass;
				bi.framebuffer = m_illum_ssgi_fbs.m_depth_resolve_fb;
				bi.renderArea = VkRect2D{{0, 0}, sex_mip};
				m_cmd.beginRenderPass(bi, VK_SUBPASS_CONTENTS_INLINE);
			}
			m_cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_r.m_depth_resolve_pipeline);
			m_cmd.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_r.m_depth_resolve_pipeline.pipelineLayout,
				0, 1, &m_illum_ssgi_fbs.m_depth_resolve_set, 0, nullptr);
			m_cmd.bindVertexBuffer(0, m_r.m_screen_vertex_buffer, 0);
			m_cmd.draw(3, 1, 0, 0);
			m_cmd.endRenderPass();

			m_cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_r.m_depth_acc_pipeline);
			m_cmd.bindVertexBuffer(0, m_r.m_screen_vertex_buffer, 0);
			for (uint32_t i = 0; i < m_illum_ssgi_fbs.m_depth_acc_fbs.size(); i++) {
				{
					VkMemoryBarrier barrier { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, Vk::Access::ColorAttachmentWriteBit, Vk::Access::ShaderReadBit };
					m_cmd.pipelineBarrier(Vk::PipelineStage::ColorAttachmentOutputBit, Vk::PipelineStage::FragmentShaderBit, 0,
						1, &barrier, 0, nullptr, 0, nullptr);
				}

				auto &cur_ex = m_r.m_swapchain_extent_mips[i + 1];
				m_cmd.setExtent(cur_ex);
				{
					VkRenderPassBeginInfo bi{};
					bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					bi.renderPass = m_r.m_depth_resolve_pass;
					bi.framebuffer = m_illum_ssgi_fbs.m_depth_acc_fbs[i];
					bi.renderArea = VkRect2D{{0, 0}, cur_ex};
					m_cmd.beginRenderPass(bi, VK_SUBPASS_CONTENTS_INLINE);
				}
				m_cmd.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_r.m_depth_acc_pipeline.pipelineLayout,
					0, 1, &m_illum_ssgi_fbs.m_depth_acc_sets[i], 0, nullptr);
				m_cmd.draw(3, 1, 0, 0);
				m_cmd.endRenderPass();
			}

			{
				VkMemoryBarrier barrier { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, Vk::Access::ColorAttachmentWriteBit, Vk::Access::ShaderReadBit };
				m_cmd.pipelineBarrier(Vk::PipelineStage::ColorAttachmentOutputBit, Vk::PipelineStage::FragmentShaderBit, 0,
					1, &barrier, 0, nullptr, 0, nullptr);
			}
			m_cmd.setExtent(m_r.m_swapchain_extent);
		}

		{
			VkRenderPassBeginInfo bi{};
			bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			bi.renderPass = m_r.m_illumination_pass;
			bi.framebuffer = m_illumination_fb;
			bi.renderArea = VkRect2D{{0, 0}, sex};
			m_cmd.beginRenderPass(bi, VK_SUBPASS_CONTENTS_INLINE);
		}
		m_cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_r.m_illumination_pipeline);
		{
			auto view_norm = camera.view;
			for (size_t i = 0; i < 3; i++)
				view_norm[3][i] = 0.0f;

			Illumination illum;
			illum.cam_proj = camera.proj;
			illum.view = camera.view;
			illum.view_normal = camera.view;
			for (size_t i = 0; i < 3; i++)
				illum.view_normal[3][i] = 0.0f;
			illum.view_inv = glm::inverse(camera.view);
			illum.view_normal_inv = illum.view_inv;
			for (size_t i = 0; i < 3; i++)
				illum.view_normal_inv[3][i] = 0.0f;
			illum.last_view = camera.last_view;
			illum.last_view_inv = glm::inverse(camera.last_view);
			illum.view_cur_to_last = illum.last_view * illum.view_inv;
			illum.view_last_to_cur = illum.view * illum.last_view_inv;
			illum.view_last_to_cur_normal = illum.view_last_to_cur;
			for (size_t i = 0; i < 3; i++)
				illum.view_last_to_cur_normal[3][i] = 0.0f;
			for (size_t i = 0; i < 256; i++) {
				reinterpret_cast<glm::vec3&>(illum.rnd_sun[i]) = genDiffuseVector(m_r, glm::normalize(glm::vec3(1.3, 3.0, 1.0)), 2000.0);
				reinterpret_cast<glm::vec3&>(illum.rnd_diffuse[i]) = genDiffuseVector(m_r, glm::vec3(0.0f, 0.0f, 1.0f), 1.0);
			}
			illum.sun = view_norm * glm::vec4(glm::normalize(glm::vec3(1.3, 3.0, 1.0)), 1.0);
			illum.size = glm::vec2(1.0f) / glm::vec2(m_r.m_swapchain_extent_mip.width, m_r.m_swapchain_extent_mip.height);
			illum.size = glm::vec2(m_r.m_swapchain_extent.width, m_r.m_swapchain_extent.height);
			illum.size_inv = glm::vec2(1.0f) / illum.size;
			illum.depth_size = glm::vec2(1.0f) / glm::vec2(m_r.m_swapchain_extent_mip.width, m_r.m_swapchain_extent_mip.height);
			illum.ratio = camera.ratio;
			illum.cam_near = camera.near;
			illum.cam_far = camera.far;
			illum.cam_a = camera.far / (camera.far - camera.near);
			illum.cam_b = -(camera.far * camera.near) / (camera.far - camera.near);


			*reinterpret_cast<Illumination*>(reinterpret_cast<uint8_t*>(m_dyn_buffer_staging_ptr) + m_dyn_buffer_size) = illum;
			{
				VkBufferCopy region {m_dyn_buffer_size, 0, sizeof(Illumination)};
				m_transfer_cmd.copyBuffer(m_dyn_buffer_staging, m_illumination_buffer, 1, &region);
			}
			m_dyn_buffer_size += sizeof(Illumination);
		}
		m_cmd.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_r.m_illumination_pipeline.pipelineLayout,
			0, 1, &m_illumination_set, 0, nullptr);
		m_cmd.bindVertexBuffer(0, m_r.m_screen_vertex_buffer, 0);
		m_cmd.draw(3, 1, 0, 0);
		m_cmd.endRenderPass();

		{
			VkMemoryBarrier barrier { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, Vk::Access::ColorAttachmentWriteBit, Vk::Access::ShaderReadBit };
			m_cmd.pipelineBarrier(Vk::PipelineStage::ColorAttachmentOutputBit, Vk::PipelineStage::FragmentShaderBit, 0,
				1, &barrier, 0, nullptr, 0, nullptr);
		}

		{
			VkRenderPassBeginInfo bi{};
			bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			bi.renderPass = m_r.m_wsi_pass;
			bi.framebuffer = m_r.m_frames[swapchain_index].m_wsi_fb;
			bi.renderArea = VkRect2D{{0, 0}, sex};
			m_cmd.beginRenderPass(bi, VK_SUBPASS_CONTENTS_INLINE);
		}
		m_cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_r.m_wsi_pipeline);
		m_cmd.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_r.m_wsi_pipeline.pipelineLayout,
			0, 1, &m_wsi_set, 0, nullptr);
		m_cmd.bindVertexBuffer(0, m_r.m_screen_vertex_buffer, 0);
		m_cmd.draw(3, 1, 0, 0);
		m_cmd.endRenderPass();

		m_cmd.end();
	}

	{
		VkMemoryBarrier barrier { VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, Vk::Access::TransferWriteBit, Vk::Access::MemoryReadBit };
		m_transfer_cmd.pipelineBarrier(Vk::PipelineStage::TransferBit, Vk::PipelineStage::AllGraphicsBit, 0,
			1, &barrier, 0, nullptr, 0, nullptr);
	}
	m_transfer_cmd.end();

	VkCommandBuffer cmds[] {
		m_transfer_cmd,
		m_cmd
	};
	VkPipelineStageFlags wait_stage = Vk::PipelineStage::ColorAttachmentOutputBit;
	VkSubmitInfo si[] {
		{VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr,
			1, m_image_ready.ptr(),
			&wait_stage,
			array_size(cmds), cmds,
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
	{
		auto pres_res = m_r.m_queue.present(pi);
		if (pres_res != VK_SUCCESS) {
			if (pres_res == VK_SUBOPTIMAL_KHR || pres_res == VK_ERROR_OUT_OF_DATE_KHR) {
				m_r.recreateSwapchain();
				return;
			} else
				vkAssert(pres_res);
		}
	}

	m_ever_submitted = true;
}

void Renderer::Frame::render_subset(Map &map, cmp_id render_id)
{
	Render cur{nullptr, nullptr, nullptr};
	size_t streak = 0;
	auto comps = sarray<cmp_id, 1>();
	comps.data()[0] = render_id;

	map.query(comps, [&](Brush &b){
		auto r = b.get<Render>(render_id);
		auto size = b.size();
		for (size_t i = 0; i < size; i++) {
			auto &n = r[i];
			if (n != cur) {
				if (streak > 0) {
					if (cur.model->indexType == VK_INDEX_TYPE_NONE_KHR)
						m_cmd.draw(cur.model->primitiveCount, streak, 0, 0);
					else
						m_cmd.drawIndexed(cur.model->primitiveCount, streak, 0, 0, 0);
					streak = 0;
				}

				if (n.pipeline != cur.pipeline)
					m_cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *n.pipeline);
				if (n.material != cur.material) {
					if (n.pipeline->pushConstantRange > 0)
						m_cmd.pushConstants(m_r.m_pipeline_layout_descriptor_set, Vk::ShaderStage::FragmentBit, 0, n.pipeline->pushConstantRange, n.material);
				}
				if (n.model != cur.model) {
					m_cmd.bindVertexBuffer(0, n.model->vertexBuffer, 0);
					if (n.model->indexType != VK_INDEX_TYPE_NONE_KHR)
						m_cmd.bindIndexBuffer(n.model->indexBuffer, 0, n.model->indexType);
				}
				{
					uint32_t dyn_off[] {static_cast<uint32_t>(m_dyn_buffer_size)};
					m_cmd.bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_r.m_pipeline_layout_descriptor_set,
						1, 1, &m_descriptor_set_dynamic, array_size(dyn_off), dyn_off);
				}
				cur = n;
			}

			auto &pip = *r[i].pipeline;
			for (size_t j = 0; j < pip.dynamicCount; j++) {
				auto dyn = pip.dynamics[j];
				auto size = Cmp::size[dyn];
				std::memcpy(reinterpret_cast<uint8_t*>(m_dyn_buffer_staging_ptr) + m_dyn_buffer_size,
					reinterpret_cast<const uint8_t*>(b.get(dyn)) + size * i, size);
				m_dyn_buffer_size += size;
			}
			streak++;
		}
	});

	if (streak > 0) {
		if (cur.model->indexType == VK_INDEX_TYPE_NONE_KHR)
			m_cmd.draw(cur.model->primitiveCount, streak, 0, 0);
		else
			m_cmd.drawIndexed(cur.model->primitiveCount, streak, 0, 0, 0);
	}
}

}