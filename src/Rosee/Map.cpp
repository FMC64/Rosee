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
		auto [it, suc] = m.emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(*this, key));
		if (!suc)
			throw std::runtime_error("Can't emplace brush");
		got = it;
	}
	return got->second;
}

std::pair<Brush*, size_t> Map::find(size_t id)
{
	auto &m = Static::get(m_ids);
	auto got = m.find(Range{id, 1});
	if (got == m.end())
		return std::pair<Brush*, size_t>(nullptr, ~0ULL);
	else
		return std::pair<Brush*, size_t>(&got->second.first, got->second.second + (id - got->first.begin));
}

}