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
struct MVP;
struct MV_normal;

namespace Cmp {

using list = List<Id, Transform, Point2D, OpaqueRender, MVP, MV_normal>;

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

struct Transform : public Cmp::Id_t<Transform>, public glm::mat4
{
	static Cmp::init_fun_t init;
	static Cmp::destr_fun_t destr;

	using glm::mat4::operator=;
};

struct Point2D : public Cmp::Id_t<Point2D>
{
	static Cmp::init_fun_t init;
	static Cmp::destr_fun_t destr;

	glm::vec3 color;
	float _pad;
	glm::vec2 pos;
	glm::vec2 base_pos;
	float size;
	float _pad2[3];
};

struct MVP : public Cmp::Id_t<MVP>, public glm::mat4
{
	static Cmp::init_fun_t init;
	static Cmp::destr_fun_t destr;

	using glm::mat4::operator=;
};

struct MV_normal : public Cmp::Id_t<MV_normal>, public glm::mat4
{
	static Cmp::init_fun_t init;
	static Cmp::destr_fun_t destr;

	using glm::mat4::operator=;
};

struct Render
{
	static Cmp::init_fun_t init;
	static Cmp::destr_fun_t destr;

	Pipeline *pipeline;
	Material *material;
	Model *model;

	inline bool operator==(const Render &other) const
	{
		return model == other.model && material == other.material && pipeline == other.pipeline;
	}

	inline bool operator!=(const Render &other) const
	{
		return !(*this == other);
	}
};

struct OpaqueRender : public Render, public Cmp::Id_t<OpaqueRender>
{
};

}

#include "Cmp/Utils.hpp"