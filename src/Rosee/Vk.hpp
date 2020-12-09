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
	Handle(void) = default;
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
	Handle& operator=(HandleType handle)
	{
		m_handle = handle;
		return *this;
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
	CommandBuffer(void) = default;
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

	void pipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
		uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
		uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
		uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers)
	{
		vkCmdPipelineBarrier(*this, srcStageMask, dstStageMask, dependencyFlags,
			memoryBarrierCount, pMemoryBarriers,
			bufferMemoryBarrierCount, pBufferMemoryBarriers,
			imageMemoryBarrierCount, pImageMemoryBarriers);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy *pRegions)
	{
		vkCmdCopyBuffer(*this, srcBuffer, dstBuffer, regionCount, pRegions);
	}

	void bindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
		uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets,
		uint32_t dynamicOffsetCount, const uint32_t *pDynamicOffsets)
	{
		vkCmdBindDescriptorSets(*this, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
	}

	void bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
	{
		vkCmdBindIndexBuffer(*this, buffer, offset, indexType);
	}

	void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
	{
		vkCmdDrawIndexed(*this, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
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
using DescriptorSetLayout = Handle<VkDescriptorSetLayout>;
using DescriptorPool = Handle<VkDescriptorPool>;
//using DescriptorSet = Handle<VkDescriptorSet>;
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

	DescriptorSetLayout createDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo &ci) const
	{
		VkDescriptorSetLayout res;
		vkAssert(vkCreateDescriptorSetLayout(*this, &ci, nullptr, &res));
		return res;
	}

	DescriptorPool createDescriptorPool(const VkDescriptorPoolCreateInfo &ci) const
	{
		VkDescriptorPool res;
		vkAssert(vkCreateDescriptorPool(*this, &ci, nullptr, &res));
		return res;
	}

	void allocateDescriptorSets(VkDescriptorPool descriptorPool,
		uint32_t desciptorSetCount, const VkDescriptorSetLayout *pSetLayouts, VkDescriptorSet *pDescriptorSets) const;

	void updateDescriptorSets(uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites,
		uint32_t descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies) const
	{
		vkUpdateDescriptorSets(*this, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
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

	void destroy(VkDescriptorSetLayout descriptorSetLayout) const
	{
		vkDestroyDescriptorSetLayout(*this, descriptorSetLayout, nullptr);
	}

	void destroy(VkDescriptorPool descriptorPool) const
	{
		vkDestroyDescriptorPool(*this, descriptorPool, nullptr);
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

namespace AttachmentLoadOp {

static inline constexpr auto Load = VK_ATTACHMENT_LOAD_OP_LOAD;
static inline constexpr auto Clear = VK_ATTACHMENT_LOAD_OP_CLEAR;
static inline constexpr auto DontCare = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

}

namespace AttachmentStoreOp {

static inline constexpr auto Store = VK_ATTACHMENT_STORE_OP_STORE;
static inline constexpr auto DontCare = VK_ATTACHMENT_STORE_OP_DONT_CARE;

}

namespace ImageLayout {

static inline constexpr auto Undefined = VK_IMAGE_LAYOUT_UNDEFINED;
static inline constexpr auto General = VK_IMAGE_LAYOUT_GENERAL;
static inline constexpr auto ColorAttachmentOptimal = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
static inline constexpr auto DepthStencilAttachmentOptimal = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
static inline constexpr auto DepthStencilReadOnlyOptimal = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
static inline constexpr auto ShaderReadOnlyOptimal = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
static inline constexpr auto TransferSrcOptimal = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
static inline constexpr auto TransferDstOptimal = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
static inline constexpr auto Preinitialized = VK_IMAGE_LAYOUT_PREINITIALIZED;
static inline constexpr auto PresentSrcKhr = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

}

static inline constexpr auto SampleCount_1Bit = VK_SAMPLE_COUNT_1_BIT;
static inline constexpr auto SampleCount_2Bit = VK_SAMPLE_COUNT_2_BIT;
static inline constexpr auto SampleCount_4Bit = VK_SAMPLE_COUNT_4_BIT;
static inline constexpr auto SampleCount_8Bit = VK_SAMPLE_COUNT_8_BIT;
static inline constexpr auto SampleCount_16Bit = VK_SAMPLE_COUNT_16_BIT;
static inline constexpr auto SampleCount_32Bit = VK_SAMPLE_COUNT_32_BIT;
static inline constexpr auto SampleCount_64Bit = VK_SAMPLE_COUNT_64_BIT;

namespace Access {

static inline constexpr auto IndirectCommandReadBit = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
static inline constexpr auto IndexReadBit = VK_ACCESS_INDEX_READ_BIT;
static inline constexpr auto VertexAttributeReadBit = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
static inline constexpr auto UniformReadBit = VK_ACCESS_UNIFORM_READ_BIT;
static inline constexpr auto InputAttachmentReadBit = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
static inline constexpr auto ShaderReadBit = VK_ACCESS_SHADER_READ_BIT;
static inline constexpr auto ShaderWriteBit = VK_ACCESS_SHADER_WRITE_BIT;
static inline constexpr auto ColorAttachmentReadBit = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
static inline constexpr auto ColorAttachmentWriteBit = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
static inline constexpr auto DepthStencilAttachmentReadBit = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
static inline constexpr auto DepthStencilAttachmentWriteBit = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
static inline constexpr auto TransferReadBit = VK_ACCESS_TRANSFER_READ_BIT;
static inline constexpr auto TransferWriteBit = VK_ACCESS_TRANSFER_WRITE_BIT;
static inline constexpr auto HostReadBit = VK_ACCESS_HOST_READ_BIT;
static inline constexpr auto HostWriteBit = VK_ACCESS_HOST_WRITE_BIT;
static inline constexpr auto MemoryReadBit = VK_ACCESS_MEMORY_READ_BIT;
static inline constexpr auto MemoryWriteBit = VK_ACCESS_MEMORY_WRITE_BIT;

}

namespace PipelineStage {

static inline constexpr auto TopOfPipeBit = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
static inline constexpr auto DrawIndirectBit = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
static inline constexpr auto VertexInputBit = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
static inline constexpr auto VertexShaderBit = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
static inline constexpr auto TesselationControlShaderBit = VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
static inline constexpr auto TesselationEvaluationShaderBit = VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
static inline constexpr auto GeometryShaderBit = VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
static inline constexpr auto FragmentShaderBit = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
static inline constexpr auto EarlyFragmentTestsBit = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
static inline constexpr auto LateFragmentTestsBit = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
static inline constexpr auto ColorAttachmentOutputBit = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
static inline constexpr auto ComputeShaderBit = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
static inline constexpr auto TransferBit = VK_PIPELINE_STAGE_TRANSFER_BIT;
static inline constexpr auto BottomOfPipeBit = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
static inline constexpr auto HostBit = VK_PIPELINE_STAGE_HOST_BIT;
static inline constexpr auto AllGraphicsBit = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
static inline constexpr auto AllCommandsBit = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

}

namespace ShaderStage {

static inline constexpr auto VertexBit = VK_SHADER_STAGE_VERTEX_BIT;
static inline constexpr auto TesselationControlBit = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
static inline constexpr auto TesselationEvaluationBit = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
static inline constexpr auto GeometryBit = VK_SHADER_STAGE_GEOMETRY_BIT;
static inline constexpr auto FragmentBit = VK_SHADER_STAGE_FRAGMENT_BIT;
static inline constexpr auto ComputeBit = VK_SHADER_STAGE_COMPUTE_BIT;
static inline constexpr auto AllGraphics = VK_SHADER_STAGE_ALL_GRAPHICS;
static inline constexpr auto All = VK_SHADER_STAGE_ALL;

}

using Allocation = Handle<VmaAllocation>;

class BufferAllocation : public Buffer, public Allocation
{
public:
	BufferAllocation(void) = default;
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

	BufferAllocation createBuffer(const VkBufferCreateInfo &bci, const VmaAllocationCreateInfo &aci) const
	{
		VkBuffer buffer;
		VmaAllocation allocation;
		vkAssert(vmaCreateBuffer(*this, &bci, &aci, &buffer, &allocation, nullptr));
		return BufferAllocation(buffer, allocation);
	}

	BufferAllocation createBuffer(const VkBufferCreateInfo &bci, const VmaAllocationCreateInfo &aci, void **ppMappedData) const
	{
		VkBuffer buffer;
		VmaAllocation allocation;
		VmaAllocationInfo alloc_info;
		vkAssert(vmaCreateBuffer(*this, &bci, &aci, &buffer, &allocation, &alloc_info));
		*ppMappedData = alloc_info.pMappedData;
		return BufferAllocation(buffer, allocation);
	}

	void destroy(BufferAllocation &bufferAllocation) const
	{
		vmaDestroyBuffer(*this, bufferAllocation, bufferAllocation);
	}

	void destroy(void)
	{
		vmaDestroyAllocator(*this);
	}

	void flushAllocation(VmaAllocation allocation, VkDeviceSize offset, VkDeviceSize size) const
	{
		vkAssert(vmaFlushAllocation(*this, allocation, offset, size));
	}

	void invalidateAllocation(VmaAllocation allocation, VkDeviceSize offset, VkDeviceSize size) const
	{
		vkAssert(vmaInvalidateAllocation(*this, allocation, offset, size));
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