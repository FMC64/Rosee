#pragma once

#include <cstdint>
#include <type_traits>
#define GLM_EXTERNAL_TEMPLATE
#include <glm/mat4x4.hpp>
#include "array.hpp"
#include "Cmp/List.hpp"
#include "Cmp/Id.hpp"

namespace Rosee {

struct Id;
struct Transform;

namespace Cmp {

using list = List<Id, Transform>;

#include "Cmp/Id_t.hpp"

}

struct Id : public Cmp::Id_t<Id>
{
	static Cmp::init_fun_t init;
	static Cmp::destr_fun_t destr;

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

struct Transform : public Cmp::Id_t<Transform>
{
	static Cmp::init_fun_t init;
	static Cmp::destr_fun_t destr;

	glm::mat4 mat;
};

}

#include "Cmp/Utils.hpp"