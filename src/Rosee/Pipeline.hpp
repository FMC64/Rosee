#pragma once

#include "Vk.hpp"
#include "Cmp.hpp"

namespace Rosee {

struct Pipeline : public Vk::Handle<VkPipeline>
{
	uint32_t shaderModuleCount = 0;
	VkShaderModule shaderModules[8];
	VkPipelineLayout pipelineLayout;
	uint32_t pushConstantRange;
	uint32_t dynamicCount = 0;
	cmp_id dynamics[8];

	void pushShaderModule(VkShaderModule module);
	void pushDynamic(cmp_id cmp);
	template <typename Component>
	void pushDynamic(void)
	{
		pushDynamic(Component::id);
	}

	using Vk::Handle<VkPipeline>::operator=;

	void destroy(Vk::Device device);
};

}