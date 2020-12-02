#include "Renderer.hpp"
#include "c.hpp"
#include <map>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>

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

Vk::Allocator Renderer::createAllocator(void)
{
	VmaAllocatorCreateInfo ci{};
	ci.vulkanApiVersion = VK_API_VERSION_1_0;
	ci.physicalDevice = m_physical_device;
	ci.device = m_device;
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
		{0, VK_FORMAT_B8G8R8A8_SRGB, Vk::SampleCount_1Bit, Vk::AttachmentLoadOp::Clear, Vk::AttachmentStoreOp::Store,	// wsi 0
			Vk::AttachmentLoadOp::DontCare, Vk::AttachmentStoreOp::DontCare,
			Vk::ImageLayout::Undefined, Vk::ImageLayout::PresentSrcKhr}
	};
	VkAttachmentReference wsi {0, Vk::ImageLayout::ColorAttachmentOptimal};
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

	auto &sex = m_swapchain_extent;

	for (auto &i : m_swapchain_image_views) {
		VkFramebufferCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		ci.renderPass = m_opaque_pass;
		ci.attachmentCount = 1;
		ci.pAttachments = i.ptr();
		ci.width = sex.width;
		ci.height = sex.height;
		ci.layers = 1;
		res.emplace(m_device.createFramebuffer(ci));
	}
	return res;
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
	return m_device.createDescriptorSetLayout(ci);
}

Vk::DescriptorPool Renderer::createDescriptorPoolDynamic(void)
{
	VkDescriptorPoolCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	ci.maxSets = m_frame_count;
	VkDescriptorPoolSize pool_sizes[] {
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, m_frame_count}
	};
	ci.poolSizeCount = array_size(pool_sizes);
	ci.pPoolSizes = pool_sizes;
	return m_device.createDescriptorPool(ci);
}

vector<Renderer::Frame> Renderer::createFrames(void)
{
	VkCommandBuffer cmds[m_frame_count * 2];
	m_device.allocateCommandBuffers(m_command_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_frame_count * 2, cmds);
	VkDescriptorSet sets[m_frame_count];
	VkDescriptorSetLayout set_layouts[m_frame_count];
	for (uint32_t i = 0; i < m_frame_count; i++)
		set_layouts[i] = m_descriptor_set_layout_dynamic;
	m_device.allocateDescriptorSets(m_descriptor_pool_dynamic, m_frame_count, set_layouts, sets);

	vector<Renderer::Frame> res;
	res.reserve(m_frame_count);
	for (uint32_t i = 0; i < m_frame_count; i++)
		res.emplace(*this, cmds[i * 2], cmds[i * 2 + 1], sets[i]);
	return res;
}

Vk::ShaderModule Renderer::loadShaderModule(const char *path) const
{
	VkShaderModuleCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	std::ifstream f(path, std::ios::binary);
	if (!f.good())
		throw std::runtime_error(path);
	std::stringstream ss;
	ss << f.rdbuf();
	auto str = ss.str();
	ci.codeSize = str.size();
	ci.pCode = reinterpret_cast<const uint32_t*>(str.data());

	return m_device.createShaderModule(ci);
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

Vk::PipelineLayout Renderer::createPipelineLayoutEmpty(void)
{
	VkPipelineLayoutCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	VkPushConstantRange ranges[] {
		{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 128}
	};
	ci.pushConstantRangeCount = array_size(ranges);
	ci.pPushConstantRanges = ranges;
	return m_device.createPipelineLayout(ci);
}

Vk::Pipeline Renderer::createParticlePipeline(void)
{
	VkGraphicsPipelineCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	VkPipelineShaderStageCreateInfo stages[] {
		initPipelineStage(VK_SHADER_STAGE_VERTEX_BIT, m_particle_vert),
		initPipelineStage(VK_SHADER_STAGE_FRAGMENT_BIT, m_particle_frag)
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
	ci.layout = m_pipeline_layout_empty;
	ci.renderPass = m_opaque_pass;

	return m_device.createGraphicsPipeline(m_pipeline_cache, ci);
}

Vk::BufferAllocation Renderer::createPointBuffer(void)
{
	VkBufferCreateInfo bci{};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.size = sizeof(glm::vec2);
	bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VmaAllocationCreateInfo aci{};
	aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	return m_allocator.createBuffer(bci, aci);
}

Renderer::Renderer(uint32_t frameCount, bool validate, bool useRenderDoc) :
	m_frame_count(frameCount),
	m_validate(validate),
	m_use_render_doc(useRenderDoc),
	m_window(createWindow()),
	m_instance(createInstance()),
	m_debug_messenger(createDebugMessenger()),
	m_surface(createSurface()),
	m_device(createDevice()),
	m_pipeline_cache(VK_NULL_HANDLE),
	m_allocator(createAllocator()),
	m_queue(m_device.getQueue(m_queue_family_graphics, 0)),
	m_swapchain(createSwapchain()),
	m_swapchain_images(m_device.getSwapchainImages(m_swapchain)),
	m_swapchain_image_views(createSwapchainImageViews()),
	m_opaque_pass(createOpaquePass()),
	m_opaque_fbs(createOpaqueFbs()),
	m_command_pool(m_device.createCommandPool(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		m_queue_family_graphics)),
	m_descriptor_set_layout_dynamic(createDescriptorSetLayoutDynamic()),
	m_descriptor_pool_dynamic(createDescriptorPoolDynamic()),
	m_frames(createFrames()),
	m_pipeline_layout_empty(createPipelineLayoutEmpty()),
	m_particle_vert(loadShaderModule("sha/particle.vert.spv")),
	m_particle_frag(loadShaderModule("sha/particle.frag.spv")),
	m_particle_pipeline(createParticlePipeline()),
	m_point_buffer(createPointBuffer())
{
	std::memset(m_keys, 0, sizeof(m_keys));
	std::memset(m_keys_prev, 0, sizeof(m_keys_prev));
}

Renderer::~Renderer(void)
{
	m_queue.waitIdle();

	m_allocator.destroy(m_point_buffer);

	m_device.destroy(m_particle_pipeline);
	m_device.destroy(m_particle_vert);
	m_device.destroy(m_particle_frag);
	m_device.destroy(m_pipeline_layout_empty);

	m_frames.clear();

	m_device.destroy(m_descriptor_pool_dynamic);
	m_device.destroy(m_descriptor_set_layout_dynamic);

	m_device.destroy(m_command_pool);
	for (auto &f : m_opaque_fbs)
		m_device.destroy(f);
	m_device.destroy(m_opaque_pass);
	for (auto &v : m_swapchain_image_views)
		m_device.destroy(v);
	m_device.destroy(m_swapchain);
	m_allocator.destroy();
	m_device.destroy();
	m_instance.destroy(m_surface);
	if (m_debug_messenger)
		m_instance.getProcAddr<PFN_vkDestroyDebugUtilsMessengerEXT>("vkDestroyDebugUtilsMessengerEXT")(m_instance, m_debug_messenger, nullptr);
	m_instance.destroy();
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Renderer::recreateSwapchain(void)
{
	m_queue.waitIdle();
	for (auto &f : m_opaque_fbs)
		m_device.destroy(f);
	for (auto &i : m_swapchain_image_views)
		m_device.destroy(i);
	m_device.destroy(m_swapchain);

	m_swapchain = createSwapchain();
	m_swapchain_images = m_device.getSwapchainImages(m_swapchain);
	m_swapchain_image_views = createSwapchainImageViews();
	m_opaque_fbs = createOpaqueFbs();
}

size_t Renderer::m_keys_update[Renderer::key_update_count] {
	GLFW_KEY_F11
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

void Renderer::resetFrame(void)
{
	m_frames[m_current_frame].reset();
}

void Renderer::render(Map &map)
{
	m_frames[m_current_frame].render(map);
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
	return m_r.m_allocator.createBuffer(bci, aci, &m_dyn_buffer_staging_ptr);
}

Vk::BufferAllocation Renderer::Frame::createDynBuffer(void)
{
	VkBufferCreateInfo bci{};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.size = dyn_buffer_size;
	bci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	VmaAllocationCreateInfo aci{};
	aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	return m_r.m_allocator.createBuffer(bci, aci);
}

Renderer::Frame::Frame(Renderer &r, VkCommandBuffer transferCmd, VkCommandBuffer cmd, VkDescriptorSet descriptorSetDynamic) :
	m_r(r),
	m_transfer_cmd(transferCmd),
	m_cmd(cmd),
	m_frame_done(r.m_device.createFence(0)),
	m_render_done(r.m_device.createSemaphore()),
	m_image_ready(r.m_device.createSemaphore()),
	m_descriptor_set_dynamic(descriptorSetDynamic),
	m_dyn_buffer_staging(createDynBufferStaging()),
	m_dyn_buffer(createDynBuffer())
{
}

Renderer::Frame::~Frame(void)
{
	m_r.m_allocator.destroy(m_dyn_buffer);
	m_r.m_allocator.destroy(m_dyn_buffer_staging);

	m_r.m_device.destroy(m_frame_done);
	m_r.m_device.destroy(m_render_done);
	m_r.m_device.destroy(m_image_ready);
}

void Renderer::Frame::reset(void)
{
	{
		std::lock_guard l(m_r.m_next_input_mutex);
		m_r.m_next_input++;
	}
	m_r.m_next_input_cv.notify_one();

	if (m_ever_submitted) {
		m_r.m_device.wait(m_frame_done);
		m_r.m_device.reset(m_frame_done);
	}
}

void Renderer::Frame::render(Map &map)
{
	std::lock_guard l(m_r.m_render_mutex);
	auto &sex = m_r.m_swapchain_extent;

	uint32_t swapchain_index;
	vkAssert(vkAcquireNextImageKHR(m_r.m_device, m_r.m_swapchain, ~0ULL, m_image_ready, VK_NULL_HANDLE, &swapchain_index));

	m_dyn_buffer_size = 0;
	m_transfer_cmd.beginPrimary(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	{
		m_cmd.beginPrimary(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		{
			VkRenderPassBeginInfo bi{};
			bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			bi.renderPass = m_r.m_opaque_pass;
			bi.framebuffer = m_r.m_opaque_fbs[swapchain_index];
			bi.renderArea = VkRect2D{{0, 0}, sex};

			VkClearColorValue cv0;
			cv0.float32[0] = 0.5f;
			cv0.float32[1] = 0.5f;
			cv0.float32[2] = 0.5f;
			cv0.float32[3] = 1.0f;
			VkClearValue cvs[] {
				{ .color = cv0 }
			};
			bi.clearValueCount = array_size(cvs);
			bi.pClearValues = cvs;
			m_cmd.beginRenderPass(bi, VK_SUBPASS_CONTENTS_INLINE);
		}

		m_cmd.setExtent(m_r.m_swapchain_extent);
		m_cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, m_r.m_particle_pipeline);
		m_cmd.bindVertexBuffer(0, m_r.m_point_buffer, 0);

		struct PC {
			glm::vec3 color;
			float _pad;
			glm::vec2 pos;
			float size;
		} pc;
		map.query<Point2D>([&](Brush &b){
			auto points = b.get<Point2D>();
			for (size_t i = 0; i < b.size(); i++) {
				pc.color = points[i].color;
				pc.pos = points[i].pos;
				pc.size = points[i].size;
				m_cmd.pushConstants(m_r.m_pipeline_layout_empty, Vk::ShaderStage::VertexBit | Vk::ShaderStage::FragmentBit, 0, sizeof(pc), &pc);
				m_cmd.draw(1, 1, 0, 0);
			}
		});

		m_cmd.endRenderPass();
		m_cmd.end();
	}

	m_r.m_allocator.flushAllocation(m_dyn_buffer_staging, 0, m_dyn_buffer_size);
	{
		VkBufferCopy region {0, 0, m_dyn_buffer_size};
		if (region.size > 0)
			m_transfer_cmd.copyBuffer(m_dyn_buffer_staging, m_dyn_buffer, 1, &region);
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
			if (pres_res == VK_SUBOPTIMAL_KHR || pres_res == VK_ERROR_OUT_OF_DATE_KHR)
				m_r.recreateSwapchain();
			else
				vkAssert(pres_res);
		}
	}

	m_ever_submitted = true;
}

}