#include "Renderer.hpp"
#include "c.hpp"
#include <map>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
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
		.fillModeNonSolid = true
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
	m_physical_device = physical_devices[chosen];

	{
		uint32_t present_mode_count;
		vkAssert(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[chosen], m_surface, &present_mode_count, nullptr));
		VkPresentModeKHR present_modes[present_mode_count];
		vkAssert(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_devices[chosen], m_surface, &present_mode_count, present_modes));
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

Vk::SwapchainKHR Renderer::createSwapchain(void)
{
	while (true) {
		vkAssert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &m_surface_capabilities));

		auto wins = getWindowSize();
		m_swapchain_extent = VkExtent2D{clamp(static_cast<uint32_t>(wins.x), max(m_surface_capabilities.minImageExtent.width, static_cast<uint32_t>(1)), m_surface_capabilities.maxImageExtent.width),
			clamp(static_cast<uint32_t>(wins.y), max(m_surface_capabilities.minImageExtent.height, static_cast<uint32_t>(1)), m_surface_capabilities.maxImageExtent.height)};
		if (m_swapchain_extent.width * m_swapchain_extent.height > 0)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
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
		{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, s0_samplers_size, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
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

Vk::DescriptorPool Renderer::createDescriptorPool(void)
{
	VkDescriptorPoolCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	ci.maxSets = m_frame_count * sets_per_frame;
	VkDescriptorPoolSize pool_sizes[] {
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, m_frame_count},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_frame_count},	// illum
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_frame_count * (
			s0_samplers_size +	// s0
			5 +			// illumination
			1			// wsi
		)}
	};
	ci.poolSizeCount = array_size(pool_sizes);
	ci.pPoolSizes = pool_sizes;
	return device.createDescriptorPool(ci);
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

Vk::RenderPass Renderer::createIlluminationPass(void)
{
	VkRenderPassCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

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

Vk::DescriptorSetLayout Renderer::createIlluminationSetLayout(void)
{
	VkDescriptorSetLayoutCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	VkDescriptorSetLayoutBinding bindings[] {
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
		{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// depth_buffer
		{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// depth
		{3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},	// albedo
		{4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}	// normal
	};
	ci.bindingCount = array_size(bindings);
	ci.pBindings = bindings;
	return device.createDescriptorSetLayout(ci);
}

Pipeline Renderer::createIlluminationPipeline(void)
{
	Pipeline res;

	VkGraphicsPipelineCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	auto frag = loadShaderModule(VK_SHADER_STAGE_FRAGMENT_BIT, "sha/illumination");
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

vector<Renderer::Frame> Renderer::createFrames(void)
{
	VkCommandBuffer cmds[m_frame_count * sets_per_frame];
	device.allocateCommandBuffers(m_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_frame_count * sets_per_frame, cmds);
	VkDescriptorSet sets[m_frame_count * sets_per_frame];
	VkDescriptorSetLayout set_layouts[m_frame_count * sets_per_frame];
	for (uint32_t i = 0; i < m_frame_count; i++) {
		set_layouts[i * sets_per_frame] = m_descriptor_set_layout_0;
		set_layouts[i * sets_per_frame + 1] = m_descriptor_set_layout_dynamic;
		set_layouts[i * sets_per_frame + 2] = m_illumination_set_layout;
		set_layouts[i * sets_per_frame + 3] = m_wsi_set_layout;
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

	vector<Renderer::Frame> res;
	res.reserve(m_frame_count);
	for (uint32_t i = 0; i < m_frame_count; i++)
		res.emplace(*this, i, cmds[i * 2], cmds[i * 2 + 1],
			sets[i * sets_per_frame], sets[i * sets_per_frame + 1], sets[i * sets_per_frame + 2], sets[i * sets_per_frame + 3],
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

VkPipelineShaderStageCreateInfo Renderer::initPipelineStage(VkShaderStageFlagBits stage, VkShaderModule module)
{
	VkPipelineShaderStageCreateInfo res{};
	res.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	res.stage = stage;
	res.module = module;
	res.pName = "main";
	return res;
}

void Pipeline::pushShaderModule(VkShaderModule module)
{
	shaderModules[shaderModuleCount++] = module;
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

Pipeline Renderer::createPipeline3D(const char *stagesPath, uint32_t pushConstantRange)
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
	if (gen_mips)
		throw std::runtime_error("not supported yet");
	ici.mipLevels = 1;
	ici.arrayLayers = 1;
	ici.samples = VK_SAMPLE_COUNT_1_BIT;
	ici.tiling = VK_IMAGE_TILING_OPTIMAL;
	ici.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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
			VkImageMemoryBarrier ibarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, Vk::Access::TransferWriteBit, Vk::Access::MemoryReadBit,
				Vk::ImageLayout::TransferDstOptimal, Vk::ImageLayout::ShaderReadOnlyOptimal, 0, 0, res,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS } };
			m_transfer_cmd.pipelineBarrier(Vk::PipelineStage::TransferBit, Vk::PipelineStage::AllGraphicsBit, 0,
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
	m_descriptor_pool(createDescriptorPool()),

	m_fwd_p2_module(loadShaderModule(VK_SHADER_STAGE_VERTEX_BIT, "sha/fwd_p2")),
	m_screen_vertex_buffer(createScreenVertexBuffer()),

	m_sample_count(fitSampleCount(VK_SAMPLE_COUNT_64_BIT)),
	m_opaque_pass(createOpaquePass()),
	m_illumination_pass(createIlluminationPass()),
	m_illumination_set_layout(createIlluminationSetLayout()),
	m_illumination_pipeline(createIlluminationPipeline()),
	m_wsi_pass(createWsiPass()),
	m_wsi_set_layout(createWsiSetLayout()),
	m_wsi_pipeline(createWsiPipeline()),

	m_sampler_fb(createSamplerFb()),

	m_frames(createFrames()),
	m_pipeline_pool(4),
	m_model_pool(4)
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

	device.destroy(m_sampler_fb);

	m_wsi_pipeline.destroy(device);
	device.destroy(m_wsi_set_layout);
	device.destroy(m_wsi_pass);
	m_illumination_pipeline.destroy(device);
	device.destroy(m_illumination_set_layout);
	device.destroy(m_illumination_pass);
	device.destroy(m_opaque_pass);

	allocator.destroy(m_screen_vertex_buffer);
	device.destroy(m_fwd_p2_module);

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
	static constexpr size_t img_writes_per_frame =
		4 +	// illum: cdepth, depth, albedo, normal
		1;	// wsi: output
	static constexpr size_t buf_writes_per_frame =
		1;	// illum: buffer
	static constexpr size_t writes_per_frame = img_writes_per_frame + buf_writes_per_frame;

	VkDescriptorImageInfo image_infos[m_frame_count * img_writes_per_frame];
	VkDescriptorBufferInfo buffer_infos[m_frame_count * buf_writes_per_frame];
	VkWriteDescriptorSet writes[m_frame_count * writes_per_frame];
	for (size_t i = 0; i < m_frame_count; i++) {
		auto &cur_frame = m_frames[i];

		struct WriteImgDesc {
			VkDescriptorSet descriptorSet;
			uint32_t binding;
			VkSampler sampler;
			VkImageView imageView;
			VkImageLayout imageLayout;
		} write_img_descs[img_writes_per_frame] {
			{cur_frame.m_illumination_set, 1, m_sampler_fb, cur_frame.m_cdepth_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
			{cur_frame.m_illumination_set, 2, m_sampler_fb, cur_frame.m_depth_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
			{cur_frame.m_illumination_set, 3, m_sampler_fb, cur_frame.m_albedo_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
			{cur_frame.m_illumination_set, 4, m_sampler_fb, cur_frame.m_normal_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
			{cur_frame.m_wsi_set, 0, m_sampler_fb, cur_frame.m_output_view, Vk::ImageLayout::ShaderReadOnlyOptimal},
		};

		for (size_t j = 0; j < img_writes_per_frame; j++) {
			auto &ii = image_infos[i * img_writes_per_frame + j];
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
			writes[i * writes_per_frame + j] = w;
		}

		struct WriteBufDesc {
			VkDescriptorSet descriptorSet;
			uint32_t binding;
			VkBuffer buffer;
		} write_buf_descs[buf_writes_per_frame] {
			{cur_frame.m_illumination_set, 0, cur_frame.m_illumination_buffer}
		};

		for (size_t j = 0; j < buf_writes_per_frame; j++) {
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
			writes[i * writes_per_frame + img_writes_per_frame + j] = w;
		}
	}
	vkUpdateDescriptorSets(device, m_frame_count * writes_per_frame, writes, 0, nullptr);
}

void Renderer::recreateSwapchain(void)
{
	m_queue.waitIdle();
	for (auto &f : m_frames)
		f.destroy();
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
	for (size_t i = 0; i < frame_count; i++) {
		auto &f = reinterpret_cast<Frame*>(frames)[i];
		m_frames.emplace(*this, i, f.m_transfer_cmd, f.m_cmd,
			f.m_descriptor_set_0, f.m_descriptor_set_dynamic,
			f.m_illumination_set, f.m_wsi_set,
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
	GLFW_KEY_ESCAPE
};

void Renderer::pollEvents(void)
{
	{
		std::unique_lock<std::mutex> l(m_next_input_mutex);
		m_next_input_cv.wait(l, [this](){
			return m_next_input > 0;
		});
		m_next_input--;
	}
	glfwPollEvents();
	{
		std::lock_guard l(m_input_mutex);
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
		std::lock_guard l(m_render_mutex);
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

bool Renderer::keyState(int glfw_key)
{
	std::lock_guard l(m_input_mutex);
	return m_keys[glfw_key];
}

bool Renderer::keyPressed(int glfw_key)
{
	std::lock_guard l(m_input_mutex);
	return !m_keys_prev[glfw_key] && m_keys[glfw_key];
}

bool Renderer::keyReleased(int glfw_key)
{
	std::lock_guard l(m_input_mutex);
	return m_keys_prev[glfw_key] && !m_keys[glfw_key];
}

glm::dvec2 Renderer::cursor(void)
{
	std::lock_guard l(m_input_mutex);
	return m_cursor;
}

void Renderer::setCursorMode(bool show)
{
	std::lock_guard l(m_input_mutex);
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
	VkDescriptorSet descriptorSetIllum, VkDescriptorSet descriptorSetWsi,
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
	m_depth_view(createFbImage(VK_FORMAT_R32_SFLOAT, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit, &m_depth)),
	m_albedo_view(createFbImageMs(VK_FORMAT_R8G8B8A8_SRGB, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit, &m_albedo)),
	m_normal_view(createFbImageMs(VK_FORMAT_R16G16B16A16_SFLOAT, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit, &m_normal)),
	m_output_view(createFbImage(VK_FORMAT_R16G16B16A16_SFLOAT, Vk::ImageAspect::ColorBit,
		Vk::ImageUsage::ColorAttachmentBit | Vk::ImageUsage::SampledBit, &m_output)),
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

	m_r.device.destroy(m_output_view);
	m_r.allocator.destroy(m_output);

	m_r.device.destroy(m_normal_view);
	m_r.allocator.destroy(m_normal);
	m_r.device.destroy(m_albedo_view);
	m_r.allocator.destroy(m_albedo);
	m_r.device.destroy(m_depth_view);
	m_r.allocator.destroy(m_depth);
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
	{
		std::lock_guard l(m_r.m_next_input_mutex);
		m_r.m_next_input++;
	}
	m_r.m_next_input_cv.notify_one();

	if (m_ever_submitted) {
		m_r.device.wait(m_frame_done);
		m_r.device.reset(m_frame_done);
	}
}

void Renderer::Frame::render(Map &map, const Camera &camera)
{
	std::lock_guard l(m_r.m_render_mutex);
	auto &sex = m_r.m_swapchain_extent;

	uint32_t swapchain_index;
	vkAssert(vkAcquireNextImageKHR(m_r.device, m_r.m_swapchain, ~0ULL, m_image_ready, VK_NULL_HANDLE, &swapchain_index));

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
			illum.sun = view_norm * glm::vec4(glm::normalize(glm::vec3(1.3, 3.0, 1.0)), 1.0);


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