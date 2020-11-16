#pragma once

#include "Brush.hpp"
#include "Static.hpp"

namespace Rosee {

class Map
{
	struct Range
	{
		size_t begin;
		size_t end;
	};

	Static::map<vector<cmp_id>, Brush> m_brushes;
	Static::map<Range, std::pair<Brush&, size_t>> m_ids;

public:
	Map(void);
	~Map(void);

private:
	Brush& brush_resolve(const vector<cmp_id> &key);

public:
	template <typename ...Components>
	Brush& brush(void)
	{
		constexpr auto sign = Cmp::make_id_array<Components...>();
		size_t fake_vec[3] = {sign.size(), 0, reinterpret_cast<size_t>(sign.data())};
		return brush_resolve(*reinterpret_cast<vector<cmp_id>*>(fake_vec));
	}

	template <typename ...Components>
	std::pair<Brush&, size_t> add(size_t count)
	{
		auto &b = brush<Components...>();
		auto res = b.add(count);
		return std::pair<Brush&, size_t>(b, res);
	}
};

}