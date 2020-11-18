#include "Renderer.hpp"
#include "c.hpp"
#include <map>
#include <iostream>

namespace Rosee {

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
	ai.apiVersion = VK_VERSION_1_0;
	ci.pApplicationInfo = &ai;

	size_t layer_count = 0;
	const char *layers[16];
	size_t ext_count = 0;
	const char *exts[16];

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

Renderer::Renderer(bool validate, bool useRenderDoc) :
	m_validate(validate),
	m_use_render_doc(useRenderDoc),
	m_instance(createInstance()),
	m_debug_messenger(createDebugMessenger())
{
}

Renderer::~Renderer(void)
{
	if (m_debug_messenger)
		m_instance.getProcAddr<PFN_vkDestroyDebugUtilsMessengerEXT>("vkDestroyDebugUtilsMessengerEXT")(m_instance, m_debug_messenger, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}


}