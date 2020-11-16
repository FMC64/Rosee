#include <stdexcept>
#include "Map.hpp"
#include "StaticMap.hpp"

namespace Rosee {

Map::Map(void)
{
}
Map::~Map(void)
{
}

Brush& Map::brush_resolve(const vector<cmp_id> &key)
{
	auto &m = Static::get(m_brushes);
	auto got = m.find(key);
	if (got == m.end()) {
		auto [it, suc] = m.emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(key));
		if (!suc)
			throw std::runtime_error("Can't emplace brush");
		got = it;
	}
	return got->second;
}

}