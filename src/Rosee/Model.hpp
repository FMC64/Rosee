#pragma once

#include "Vk.hpp"

namespace Rosee {

struct Model
{
	size_t primitiveCount;
	Vk::BufferAllocation vertexBuffer;
	Vk::BufferAllocation indexBuffer;
	VkIndexType indexType;

	void destroy(Vk::Allocator allocator);

};

}