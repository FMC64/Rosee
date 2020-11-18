#pragma once

#include <cstdint>
#include <type_traits>
#define GLM_EXTERNAL_TEMPLATE
#include <glm/mat4x4.hpp>
#include "array.hpp"
#include "CmpList.hpp"
#include "CmpId.hpp"

namespace Rosee {

struct Id;
struct Transform;

namespace Cmp {

using list = List<Id, Transform>;

}

struct Id
{
	static inline constexpr cmp_id id = Cmp::list::get_id<Id>();
	static void init(void *ptr, size_t size);
	static void destr(void *ptr, size_t size);

	using type = uint32_t;

	type value;

	Id& operator=(type val)
	{
		value = val;
		return *this;
	}

	operator type(void) const
	{
		return value;
	}
};

struct Transform
{
	static inline constexpr cmp_id id = Cmp::list::get_id<Transform>();
	static void init(void *ptr, size_t size);
	static void destr(void *ptr, size_t size);

	glm::mat4 mat;
};

}

#include "CmpUtils.hpp"