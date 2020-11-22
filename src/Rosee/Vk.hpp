#pragma once

#include <vulkan/vulkan.h>
#include "../../dep/VulkanMemoryAllocator/src/vk_mem_alloc.h"
#include "vector.hpp"

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

	HandleType* ptr(void) { return &m_handle; }
	const HandleType* ptr(void) const { return &m_handle; }
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

static inline Instance createInstance(const VkInstanceCreateInfo &ci)
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

using SwapchainKHR = Handle<VkSwapchainKHR>;
using RenderPass = Handle<VkRenderPass>;

class CommandPool : public Handle<VkCommandPool>
{
public:
	CommandPool(VkCommandPool commandPool) :
		Handle<VkCommandPool>(commandPool)
	{
	}
};

class CommandBuffer : public Handle<VkCommandBuffer>
{
public:
	CommandBuffer(VkCommandBuffer commandBuffer) :
		Handle<VkCommandBuffer>(commandBuffer)
	{
	}

	void beginPrimary(VkCommandBufferUsageFlags flags);
	void end(void);

	void beginRenderPass(const VkRenderPassBeginInfo &bi, VkSubpassContents contents)
	{
		vkCmdBeginRenderPass(*this, &bi, contents);
	}

	void endRenderPass(void)
	{
		vkCmdEndRenderPass(*this);
	}

	void bindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
	{
		vkCmdBindPipeline(*this, pipelineBindPoint, pipeline);
	}

	void bindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets)
	{
		vkCmdBindVertexBuffers(*this, firstBinding, bindingCount, pBuffers, pOffsets);
	}

	void bindVertexBuffer(uint32_t firstBinding, VkBuffer buffer, VkDeviceSize offset)
	{
		vkCmdBindVertexBuffers(*this, firstBinding, 1, &buffer, &offset);
	}

	void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		vkCmdDraw(*this, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void setViewport(const VkViewport &viewport)
	{
		vkCmdSetViewport(*this, 0, 1, &viewport);
	}

	void setScissor(const VkRect2D &scissor)
	{
		vkCmdSetScissor(*this, 0, 1, &scissor);
	}

	void setExtent(const VkExtent2D &extent)
	{
		setViewport(VkViewport{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f});
		setScissor(VkRect2D{{0, 0}, {extent.width, extent.height}});
	}

	void pushConstants(VkPipelineLayout pipelineLayout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *pValues)
	{
		vkCmdPushConstants(*this, pipelineLayout, stageFlags, offset, size, pValues);
	}
};

class Queue : public Handle<VkQueue>
{
public:
	Queue(VkQueue queue) :
		Handle<VkQueue>(queue)
	{
	}

	void waitIdle(void) const;

	void submit(uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence)
	{
		vkAssert(vkQueueSubmit(*this, submitCount, pSubmits, fence));
	}

	// dont assert because suboptimal must be handled
	VkResult present(const VkPresentInfoKHR &pi) const
	{
		return vkQueuePresentKHR(*this, &pi);
	}
};

class Fence : public Handle<VkFence>
{
public:
	Fence(VkFence fence) :
		Handle<VkFence>(fence)
	{
	}
};

class Semaphore : public Handle<VkSemaphore>
{
public:
	Semaphore(VkSemaphore semaphore) :
		Handle<VkSemaphore>(semaphore)
	{
	}
};

using Framebuffer = Handle<VkFramebuffer>;
using ImageView = Handle<VkImageView>;
using PipelineCache = Handle<VkPipelineCache>;
using ShaderModule = Handle<VkShaderModule>;
using PipelineLayout = Handle<VkPipelineLayout>;
using Pipeline = Handle<VkPipeline>;
using Buffer = Handle<VkBuffer>;

class Device : public Handle<VkDevice>
{
public:
	Device(VkDevice dev) :
		Handle<VkDevice>(dev)
	{
	}

	vector<VkImage> getSwapchainImages(VkSwapchainKHR swapchain) const;

	Queue getQueue(uint32_t family, uint32_t index) const;
	void wait(VkFence fence) const;
	void reset(VkFence fence);

	SwapchainKHR createSwapchainKHR(const VkSwapchainCreateInfoKHR &ci) const
	{
		VkSwapchainKHR res;
		vkAssert(vkCreateSwapchainKHR(*this, &ci, nullptr, &res));
		return res;
	}

	RenderPass createRenderPass(const VkRenderPassCreateInfo &ci) const
	{
		VkRenderPass res;
		vkAssert(vkCreateRenderPass(*this, &ci, nullptr, &res));
		return res;
	}

	CommandPool createCommandPool(VkCommandPoolCreateFlags flags, uint32_t queueFamilyIndex) const;

	void allocateCommandBuffers(VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t commandBufferCount, VkCommandBuffer *commandBuffers) const;

	Fence createFence(VkFenceCreateFlags flags) const;
	Semaphore createSemaphore(void) const;

	Framebuffer createFramebuffer(const VkFramebufferCreateInfo &ci) const
	{
		VkFramebuffer res;
		vkAssert(vkCreateFramebuffer(*this, &ci, nullptr, &res));
		return res;
	}

	ImageView createImageView(const VkImageViewCreateInfo &ci) const
	{
		VkImageView res;
		vkAssert(vkCreateImageView(*this, &ci, nullptr, &res));
		return res;
	}

	void createGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, VkPipeline *pPipelines) const
	{
		vkAssert(vkCreateGraphicsPipelines(*this, pipelineCache, createInfoCount, pCreateInfos, nullptr, pPipelines));
	}

	ShaderModule createShaderModule(const VkShaderModuleCreateInfo &ci) const
	{
		VkShaderModule res;
		vkAssert(vkCreateShaderModule(*this, &ci, nullptr, &res));
		return res;
	}

	PipelineLayout createPipelineLayout(const VkPipelineLayoutCreateInfo &ci) const
	{
		VkPipelineLayout res;
		vkAssert(vkCreatePipelineLayout(*this, &ci, nullptr, &res));
		return res;
	}

	Pipeline createGraphicsPipeline(VkPipelineCache pipelineCache, const VkGraphicsPipelineCreateInfo &ci) const
	{
		VkPipeline res;
		vkAssert(vkCreateGraphicsPipelines(*this, pipelineCache, 1, &ci, nullptr, &res));
		return res;
	}

	Buffer createBuffer(VkBufferCreateInfo &ci) const
	{
		VkBuffer res;
		vkAssert(vkCreateBuffer(*this, &ci, nullptr, &res));
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

	void destroy(VkCommandPool commandPool) const
	{
		vkDestroyCommandPool(*this, commandPool, nullptr);
	}

	void destroy(VkFence fence) const
	{
		vkDestroyFence(*this, fence, nullptr);
	}

	void destroy(VkSemaphore semaphore) const
	{
		vkDestroySemaphore(*this, semaphore, nullptr);
	}

	void destroy(VkFramebuffer framebuffer) const
	{
		vkDestroyFramebuffer(*this, framebuffer, nullptr);
	}

	void destroy(VkImageView imageView) const
	{
		vkDestroyImageView(*this, imageView, nullptr);
	}

	void destroy(VkShaderModule shaderModule) const
	{
		vkDestroyShaderModule(*this, shaderModule, nullptr);
	}

	void destroy(VkPipelineLayout pipelineLayout) const
	{
		vkDestroyPipelineLayout(*this, pipelineLayout, nullptr);
	}

	void destroy(VkPipeline pipeline) const
	{
		vkDestroyPipeline(*this, pipeline, nullptr);
	}

	void destroy(VkBuffer buffer) const
	{
		vkDestroyBuffer(*this, buffer, nullptr);
	}

	void destroy(void)
	{
		vkDestroyDevice(*this, nullptr);
	}
};

static inline Vk::Device createDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo &ci)
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

using Allocation = Handle<VmaAllocation>;

class BufferAllocation : public Buffer, public Allocation
{
public:
	BufferAllocation(VkBuffer buffer, VmaAllocation allocation) :
		Buffer(buffer),
		Allocation(allocation)
	{
	}
};

class Allocator : public Handle<VmaAllocator>
{
public:
	Allocator(VmaAllocator allocator) :
		Handle<VmaAllocator>(allocator)
	{
	}

	BufferAllocation createBuffer(const VkBufferCreateInfo &bci, const VmaAllocationCreateInfo &aci)
	{
		VkBuffer buffer;
		VmaAllocation allocation;
		vkAssert(vmaCreateBuffer(*this, &bci, &aci, &buffer, &allocation, nullptr));
		return BufferAllocation(buffer, allocation);
	}

	void destroy(BufferAllocation &bufferAllocation)
	{
		vmaDestroyBuffer(*this, bufferAllocation, bufferAllocation);
	}

	void destroy(void)
	{
		vmaDestroyAllocator(*this);
	}
};

static inline Vk::Allocator createAllocator(const VmaAllocatorCreateInfo &ci)
{
	VmaAllocator res;
	vkAssert(vmaCreateAllocator(&ci, &res));
	return res;
}

}
}