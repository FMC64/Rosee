#include "Vk.hpp"
#include <map>
#include <stdexcept>

namespace Rosee {

void vkAssert(VkResult res)
{
	static const std::map<VkResult, const char*> table {
		{VK_SUCCESS, "VK_SUCCESS"},
		{VK_NOT_READY, "VK_NOT_READY"},
		{VK_TIMEOUT, "VK_TIMEOUT"},
		{VK_EVENT_SET, "VK_EVENT_SET"},
		{VK_EVENT_RESET, "VK_EVENT_RESET"},
		{VK_INCOMPLETE, "VK_INCOMPLETE"},
		{VK_ERROR_OUT_OF_HOST_MEMORY, "VK_ERROR_OUT_OF_HOST_MEMORY"},
		{VK_ERROR_OUT_OF_DEVICE_MEMORY, "VK_ERROR_OUT_OF_DEVICE_MEMORY"},
		{VK_ERROR_INITIALIZATION_FAILED, "VK_ERROR_INITIALIZATION_FAILED"},
		{VK_ERROR_DEVICE_LOST, "VK_ERROR_DEVICE_LOST"},
		{VK_ERROR_MEMORY_MAP_FAILED, "VK_ERROR_MEMORY_MAP_FAILED"},
		{VK_ERROR_LAYER_NOT_PRESENT, "VK_ERROR_LAYER_NOT_PRESENT"},
		{VK_ERROR_EXTENSION_NOT_PRESENT, "VK_ERROR_EXTENSION_NOT_PRESENT"},
		{VK_ERROR_FEATURE_NOT_PRESENT, "VK_ERROR_FEATURE_NOT_PRESENT"},
		{VK_ERROR_INCOMPATIBLE_DRIVER, "VK_ERROR_INCOMPATIBLE_DRIVER"},
		{VK_ERROR_TOO_MANY_OBJECTS, "VK_ERROR_TOO_MANY_OBJECTS"},
		{VK_ERROR_FORMAT_NOT_SUPPORTED, "VK_ERROR_FORMAT_NOT_SUPPORTED"},
		{VK_ERROR_FRAGMENTED_POOL, "VK_ERROR_FRAGMENTED_POOL"},
		{VK_ERROR_UNKNOWN, "VK_ERROR_UNKNOWN"},
		{VK_ERROR_OUT_OF_POOL_MEMORY, "VK_ERROR_OUT_OF_POOL_MEMORY"},
		{VK_ERROR_INVALID_EXTERNAL_HANDLE, "VK_ERROR_INVALID_EXTERNAL_HANDLE"},
		{VK_ERROR_FRAGMENTATION, "VK_ERROR_FRAGMENTATION"},
		{VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS, "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS"},
		{VK_ERROR_SURFACE_LOST_KHR, "VK_ERROR_SURFACE_LOST_KHR"},
		{VK_ERROR_NATIVE_WINDOW_IN_USE_KHR, "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR"},
		{VK_SUBOPTIMAL_KHR, "VK_SUBOPTIMAL_KHR"},
		{VK_ERROR_OUT_OF_DATE_KHR, "VK_SUBOPTIMAL_KHR"},
		{VK_ERROR_INCOMPATIBLE_DISPLAY_KHR, "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR"},
		{VK_ERROR_VALIDATION_FAILED_EXT, "VK_ERROR_VALIDATION_FAILED_EXT"},
		{VK_ERROR_INVALID_SHADER_NV, "VK_ERROR_INVALID_SHADER_NV"},
		{VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT, "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT"},
		{VK_ERROR_NOT_PERMITTED_EXT, "VK_ERROR_NOT_PERMITTED_EXT"},
		{VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT, "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT"}
	};

	if (res != VK_SUCCESS)
		throw std::runtime_error(table.at(res));
}

void* Vk::Instance::getProcAddrImpl(const char *name) const
{
	auto res = vkGetInstanceProcAddr(*this, name);

	if (res == nullptr)
		throw std::runtime_error(std::string("Can't resolve proc '") + std::string(name) + std::string("'"));
	return reinterpret_cast<void*>(res);
}

vector<VkImage> Vk::Device::getSwapchainImages(VkSwapchainKHR swapchain) const
{
	uint32_t count;
	vkAssert(vkGetSwapchainImagesKHR(*this, swapchain, &count, nullptr));
	vector<VkImage> res(count);
	vkAssert(vkGetSwapchainImagesKHR(*this, swapchain, &count, res.data()));
	return res;
}

Vk::Queue Vk::Device::getQueue(uint32_t family, uint32_t index) const
{
	VkQueue res;
	vkGetDeviceQueue(*this, family, index, &res);
	return res;
}

void Vk::Queue::waitIdle(void) const
{
	vkQueueWaitIdle(*this);
}

Vk::CommandPool Vk::Device::createCommandPool(VkCommandPoolCreateFlags flags, uint32_t queueFamilyIndex) const
{
	VkCommandPoolCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	ci.flags = flags;
	ci.queueFamilyIndex = queueFamilyIndex;

	VkCommandPool res;
	vkAssert(vkCreateCommandPool(*this, &ci, nullptr, &res));
	return res;
}

void Vk::CommandBuffer::beginPrimary(VkCommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo bi{};
	bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bi.flags = flags;
	vkAssert(vkBeginCommandBuffer(*this, &bi));
}

void Vk::CommandBuffer::end(void)
{
	vkAssert(vkEndCommandBuffer(*this));
}

void Vk::Device::wait(VkFence fence) const
{
	vkAssert(vkWaitForFences(*this, 1, &fence, VK_TRUE, ~0ULL));
}

void Vk::Device::reset(VkFence fence)
{
	vkAssert(vkResetFences(*this, 1, &fence));
}

void Vk::Device::allocateCommandBuffers(VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t commandBufferCount, VkCommandBuffer *commandBuffers) const
{
	VkCommandBufferAllocateInfo ai{};
	ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	ai.commandPool = commandPool;
	ai.level = level;
	ai.commandBufferCount = commandBufferCount;
	vkAssert(vkAllocateCommandBuffers(*this, &ai, commandBuffers));
}

Vk::Fence Vk::Device::createFence(VkFenceCreateFlags flags) const
{
	VkFenceCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	ci.flags = flags;

	VkFence res;
	vkAssert(vkCreateFence(*this, &ci, nullptr, &res));
	return res;
}

Vk::Semaphore Vk::Device::createSemaphore(void) const
{
	VkSemaphoreCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore res;
	vkAssert(vkCreateSemaphore(*this, &ci, nullptr, &res));
	return res;
}

}