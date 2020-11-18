#pragma once

#include "CmpId.hpp"

namespace Rosee {
namespace Cmp {

static inline constexpr cmp_id max = list::get_size();
extern size_t size[max];

extern init_t init[max];
extern destr_t destr[max];

template <typename First, typename ...Rest>
constexpr void __make_id_array(cmp_id *to_write)
{
	*to_write = First::id;
	if constexpr (sizeof...(Rest) > 0)
		__make_id_array<Rest...>(to_write + 1);
};

template <typename ...Components>
static constexpr auto make_id_array(void)
{
	auto res = sarray<cmp_id, sizeof...(Components)>();

	if constexpr (sizeof...(Components) > 0)
		__make_id_array<Components...>(res.cdata());
	res.sort();
	return res;
}

}
}