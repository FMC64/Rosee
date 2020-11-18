#pragma once

#include <vulkan/vulkan.h>

namespace Rosee {

void vkAssert(VkResult res);

namespace Vk {

template <typename HandleType>
class Handle
{
	HandleType m_handle;

public:
	Handle(HandleType handle) :
		m_handle(handle)
	{
	}

	operator HandleType(void) const
	{
		return m_handle;
	}

	
};

class Instance : public Handle<VkInstance>
{
public:
	Instance(VkInstance instance) :
		Handle<VkInstance>(instance)
	{
	}
};

}
}