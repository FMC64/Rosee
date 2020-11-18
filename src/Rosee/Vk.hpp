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

	operator bool(void) const
	{
		return m_handle != VK_NULL_HANDLE;
	}
};

using SurfaceKHR = Handle<VkSurfaceKHR>;

class Instance : public Handle<VkInstance>
{
public:
	Instance(VkInstance instance) :
		Handle<VkInstance>(instance)
	{
	}

	void destroy(VkSurfaceKHR surface) const
	{
		vkDestroySurfaceKHR(*this, surface, nullptr);
	}

	void destroy(void)
	{
		vkDestroyInstance(*this, nullptr);
	}

private:
	void* getProcAddrImpl(const char *name) const;

public:
	template <typename ProcType>
	ProcType getProcAddr(const char *name) const
	{
		return reinterpret_cast<ProcType>(getProcAddrImpl(name));
	}
};

static inline Instance createInstance(VkInstanceCreateInfo &ci)
{
	VkInstance res;
	vkAssert(vkCreateInstance(&ci, nullptr, &res));
	return res;
}

class DebugUtilsMessengerEXT : public Handle<VkDebugUtilsMessengerEXT>
{
public:
	DebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT messenger) :
		Handle<VkDebugUtilsMessengerEXT>(messenger)
	{
	}
};

using Queue = Handle<VkQueue>;
using SwapchainKHR = Handle<VkSwapchainKHR>;
using RenderPass = Handle<VkRenderPass>;

class Device : public Handle<VkDevice>
{
public:
	Device(VkDevice dev) :
		Handle<VkDevice>(dev)
	{
	}

	Queue getQueue(uint32_t family, uint32_t index) const
	{
		VkQueue res;
		vkGetDeviceQueue(*this, family, index, &res);
		return res;
	}

	SwapchainKHR createSwapchainKHR(VkSwapchainCreateInfoKHR &ci) const
	{
		VkSwapchainKHR res;
		vkAssert(vkCreateSwapchainKHR(*this, &ci, nullptr, &res));
		return res;
	}

	RenderPass createRenderPass(VkRenderPassCreateInfo &ci) const
	{
		VkRenderPass res;
		vkAssert(vkCreateRenderPass(*this, &ci, nullptr, &res));
		return res;
	}

	void destroy(VkRenderPass renderPass) const
	{
		vkDestroyRenderPass(*this, renderPass, nullptr);
	}

	void destroy(VkSwapchainKHR swapchain) const
	{
		vkDestroySwapchainKHR(*this, swapchain, nullptr);
	}

	void destroy(void)
	{
		vkDestroyDevice(*this, nullptr);
	}
};

static inline Vk::Device createDevice(VkPhysicalDevice physicalDevice, VkDeviceCreateInfo &ci)
{
	VkDevice res;
	vkAssert(vkCreateDevice(physicalDevice, &ci, nullptr, &res));
	return res;
}

static inline constexpr auto AttachmentLoadOp_Load = VK_ATTACHMENT_LOAD_OP_LOAD;
static inline constexpr auto AttachmentLoadOp_Clear = VK_ATTACHMENT_LOAD_OP_CLEAR;
static inline constexpr auto AttachmentLoadOp_DontCare = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

static inline constexpr auto AttachmentStoreOp_Store = VK_ATTACHMENT_STORE_OP_STORE;
static inline constexpr auto AttachmentStoreOp_DontCare = VK_ATTACHMENT_STORE_OP_DONT_CARE;

static inline constexpr auto ImageLayout_Undefined = VK_IMAGE_LAYOUT_UNDEFINED;
static inline constexpr auto ImageLayout_General = VK_IMAGE_LAYOUT_GENERAL;
static inline constexpr auto ImageLayout_ColorAttachmentOptimal = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
static inline constexpr auto ImageLayout_DepthStencilAttachmentOptimal = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
static inline constexpr auto ImageLayout_DepthStencilReadOnlyOptimal = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
static inline constexpr auto ImageLayout_ShaderReadOnlyOptimal = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
static inline constexpr auto ImageLayout_TransferSrcOptimal = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
static inline constexpr auto ImageLayout_TransferDstOptimal = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
static inline constexpr auto ImageLayout_Preinitialized = VK_IMAGE_LAYOUT_PREINITIALIZED;
static inline constexpr auto ImageLayout_PresentSrcKhr = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

static inline constexpr auto SampleCount1Bit = VK_SAMPLE_COUNT_1_BIT;
static inline constexpr auto SampleCount2Bit = VK_SAMPLE_COUNT_2_BIT;
static inline constexpr auto SampleCount4Bit = VK_SAMPLE_COUNT_4_BIT;
static inline constexpr auto SampleCount8Bit = VK_SAMPLE_COUNT_8_BIT;
static inline constexpr auto SampleCount16Bit = VK_SAMPLE_COUNT_16_BIT;
static inline constexpr auto SampleCount32Bit = VK_SAMPLE_COUNT_32_BIT;
static inline constexpr auto SampleCount64Bit = VK_SAMPLE_COUNT_64_BIT;

}
}