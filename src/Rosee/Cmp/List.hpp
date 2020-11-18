#pragma once

#include "Id.hpp"
#include "../array.hpp"

namespace Rosee {
namespace Cmp {

template <typename ...Components>
struct List
{
	template <typename Component>
	static constexpr cmp_id get_id(void)
	{
		static_assert(sizeof...(Components) > 0, "Search for id but cmp list is empty");
		return get_id_imp<Component, 0, Components...>();
	}

	static constexpr size_t get_size(void)
	{
		return sizeof...(Components);
	}

	static constexpr void fill_sizes(size_t *to_write)
	{
		if constexpr (sizeof...(Components) > 0)
			fill_size<Components...>(to_write);
	}

	static constexpr void fill_inits(init_t *to_write)
	{
		if constexpr (sizeof...(Components) > 0)
			fill_init<Components...>(to_write);
	}
	static constexpr void fill_destrs(destr_t *to_write)
	{
		if constexpr (sizeof...(Components) > 0)
			fill_destr<Components...>(to_write);
	}

private:
	template <typename Component, size_t Id, typename First, typename ...Rest>
	static constexpr cmp_id get_id_imp(void)
	{
		if constexpr (std::is_same_v<Component, First>)
			return Id;
		else {
			static_assert(sizeof...(Rest) > 0, "Search for id but rest list is empty");
			return get_id_imp<Component, Id + 1, Rest...>();
		}
	}

	template <typename First, typename ...Rest>
	static constexpr void fill_size(size_t *to_write)
	{
		*to_write = sizeof(First);
		if constexpr (sizeof...(Rest) > 0)
			fill_size<Rest...>(to_write + 1);
	};

	template <typename First, typename ...Rest>
	static constexpr void fill_init(init_t *to_write)
	{
		*to_write = First::init;
		if constexpr (sizeof...(Rest) > 0)
			fill_init<Rest...>(to_write + 1);
	};

	template <typename First, typename ...Rest>
	static constexpr void fill_destr(destr_t *to_write)
	{
		*to_write = First::destr;
		if constexpr (sizeof...(Rest) > 0)
			fill_destr<Rest...>(to_write + 1);
	};
};

}
}