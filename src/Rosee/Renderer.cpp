#include "Renderer.hpp"
#include "c.hpp"

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

Renderer::Renderer(bool validate, bool useRenderDoc) :
	m_validate(validate),
	m_use_render_doc(useRenderDoc),
	m_instance(createInstance())
{
}

}