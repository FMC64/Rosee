#pragma once

#include <cstdint>
#include <glm/mat4x4.hpp>
#include "array.hpp"

namespace Rosee {

using cmp_id = size_t;

struct Id
{
	static inline constexpr cmp_id id = 0;
	static void init(void *ptr, size_t size);
	static void destr(void *ptr, size_t size);

	uint32_t value;
};

struct Transform
{
	static inline constexpr cmp_id id = 1;
	static void init(void *ptr, size_t size);
	static void destr(void *ptr, size_t size);

	glm::mat4 mat;
};

namespace Cmp {

using last = Transform;	// keep that updated
static inline constexpr cmp_id max = last::id + 1;
extern size_t size[max];

using init_t = void (void *ptr, size_t size);
using destr_t = void (void *ptr, size_t size);
extern init_t *init[max];
extern destr_t *destr[max];

template <typename First, typename ...Rest>
void __make_id_array(cmp_id *to_write)
{
	*to_write = First::id;
	if constexpr (sizeof...(Rest) > 0)
		__make_id_array<Rest...>(to_write + 1);
};

template <typename ...Components>
static auto make_id_array(void)
{
	auto res = sarray<cmp_id, sizeof...(Components)>();

	if constexpr (sizeof...(Components) > 0)
		__make_id_array<Components...>(res.data());
	return res;
}

}
}