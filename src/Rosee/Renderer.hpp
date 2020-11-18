#pragma once

#include "Vk.hpp"

namespace Rosee {

class Renderer
{
	bool m_validate;
	bool m_use_render_doc;
	Vk::Instance m_instance;
	Vk::Instance createInstance(void);
	Vk::DebugUtilsMessengerEXT m_debug_messenger;
	Vk::DebugUtilsMessengerEXT createDebugMessenger(void);
	VkPhysicalDeviceProperties m_properties;
	VkPhysicalDeviceLimits m_limits;
	VkPhysicalDeviceFeatures m_features;
	Vk::Device m_device;
	Vk::Device createDevice(void);

public:
	Renderer(bool validate, bool useRenderDoc);
	~Renderer(void);
};

}