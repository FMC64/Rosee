#pragma once

#include <cstdint>
#include <type_traits>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include "array.hpp"
#include "Cmp/List.hpp"
#include "Cmp/Id.hpp"

namespace Rosee {

struct Pipeline;
struct Material;
struct Model;

struct Id;
struct Transform;
struct Point2D;
struct OpaqueRender;

namespace Cmp {

using list = List<Id, Transform, Point2D, OpaqueRender>;

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

struct Point2D : public Cmp::Id_t<Point2D>
{
	static Cmp::init_fun_t init;
	static Cmp::destr_fun_t destr;

	glm::vec3 color;
	glm::vec2 pos;
	glm::vec2 base_pos;
	float size;
};

struct Render
{
	static Cmp::init_fun_t init;
	static Cmp::destr_fun_t destr;

	Pipeline *pipeline;
	Material *material;
	Model *model;
};

struct OpaqueRender : public Render, public Cmp::Id_t<OpaqueRender>
{
};

}

#include "Cmp/Utils.hpp"