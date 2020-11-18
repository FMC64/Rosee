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
	auto res = glfwCreateWindow(1600, 900, "Rosee", nullptr, nullptr);
	if (res == nullptr)
		throwGlfwError();
	return res;
}

Vk::Instance Renderer::createInstance(void)
{
	VkInstanceCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

	VkApplicationInfo ai{};
	ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ai.pApplicationName = "SUNREN";
	ai.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	ai.pEngineName = "Rosee";
	ai.engineVersion = VK_MAKE_VERSION(0, 0, 0);
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

	VkInstance res;
	vkAssert(vkCreateInstance(&ci, nullptr, &res));
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

Vk::Device Renderer::createDevice(void)
{
	uint32_t physical_device_count;
	vkAssert(vkEnumeratePhysicalDevices(m_instance, &physical_device_count, nullptr));
	VkPhysicalDevice physical_devices[physical_device_count];
	VkPhysicalDeviceProperties physical_devices_properties[physical_device_count];
	VkPhysicalDeviceFeatures physical_devices_features[physical_device_count];
	vkAssert(vkEnumeratePhysicalDevices(m_instance, &physical_device_count, physical_devices));

	for (uint32_t i = 0; i < physical_device_count; i++) {
		vkGetPhysicalDeviceProperties(physical_devices[i], &physical_devices_properties[i]);
		vkGetPhysicalDeviceFeatures(physical_devices[i], &physical_devices_features[i]);
	}

	static auto required_features = VkPhysicalDeviceFeatures {
	};

	uint32_t chosen = ~0U;
	size_t chosen_score = ~0ULL;

	for (size_t i = 0; i < physical_device_count; i++) {
		//auto &dev = physical_devices[i];
		auto &properties = physical_devices_properties[i];
		auto &features = physical_devices_features[i];

		constexpr size_t f_b_count = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
		auto req_f_set = reinterpret_cast<VkBool32*>(&required_features);
		auto got_f_set = reinterpret_cast<VkBool32*>(&features);
		for (size_t i = 0; i < f_b_count; i++)
			if (req_f_set[i] && !got_f_set[i])
				continue;

		size_t score = 1;
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score++;
		if (chosen == ~0U || score > chosen_score) {
			chosen = i;
			chosen_score = score;
		}

		//vkEnumerateDeviceExtensionProperties
	};

	if (chosen == ~0U)
		throw std::runtime_error("No compatible GPU found on your system.");
	m_properties = physical_devices_properties[chosen];
	m_limits = m_properties.limits;
	m_features = physical_devices_features[chosen];

	VkDeviceCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	VkDeviceQueueCreateInfo qci{};
	qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	qci.queueFamilyIndex = 0;
	qci.queueCount = 1;

	const float prio[] {
		1.0f
	};
	qci.pQueuePriorities = prio;

	VkDeviceQueueCreateInfo qcis[] {qci};
	ci.queueCreateInfoCount = array_size(qcis);
	ci.pQueueCreateInfos = qcis;

	VkDevice res;
	vkAssert(vkCreateDevice(physical_devices[chosen], &ci, nullptr, &res));
	return res;
}

Renderer::Renderer(bool validate, bool useRenderDoc) :
	m_validate(validate),
	m_use_render_doc(useRenderDoc),
	m_window(createWindow()),
	m_instance(createInstance()),
	m_debug_messenger(createDebugMessenger()),
	m_device(createDevice())
{
}

Renderer::~Renderer(void)
{
	vkDestroyDevice(m_device, nullptr);
	if (m_debug_messenger)
		m_instance.getProcAddr<PFN_vkDestroyDebugUtilsMessengerEXT>("vkDestroyDebugUtilsMessengerEXT")(m_instance, m_debug_messenger, nullptr);
	vkDestroyInstance(m_instance, nullptr);
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

}