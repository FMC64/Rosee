#include <stdexcept>
#include "Map.hpp"
#include "StaticMap.hpp"
#include "math.hpp"

namespace Rosee {

Map::Map(void)
{
}
Map::~Map(void)
{
}

void Map::add_range(size_t begin, size_t end, Brush &b, size_t b_ndx)
{
	auto &ranges = Static::get(m_ids);

	auto [it, suc] = ranges.emplace(std::piecewise_construct, std::forward_as_tuple(Map::Range{begin, end}), std::forward_as_tuple(b, b_ndx));
	if (!suc)
		throw std::runtime_error("Can't emplace in brush");
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
	auto got = m.find(Range{id, id + 1});
	if (got == m.end())
		return std::pair<Brush*, size_t>(nullptr, ~0ULL);
	else
		return std::pair<Brush*, size_t>(&got->second.first, got->second.second + (id - got->first.begin));
}

void Map::remove(size_t id, size_t count)
{
	auto &m = Static::get(m_ids);
	while (count > 0) {
		auto cur_range = Range{id, id + count};
		auto got = m.find(Range{id, id + 1});
		if (got == m.end())
			throw std::runtime_error("Can't find entity for destruction");
		auto g_range = got->first;
		auto g_pos = got->second;
		m.erase(got);
		bool room_l = g_range.begin < cur_range.begin;
		bool room_r = cur_range.end < g_range.end;
		size_t off = cur_range.begin - g_range.begin;
		size_t end = min(cur_range.end, g_range.end);
		size_t size = end - cur_range.begin;
		count -= size;
		id += size;
		if (room_l)
			add_range(g_range.begin, cur_range.begin, g_pos.first, g_pos.second);
		if (room_r)
			add_range(cur_range.end, g_range.end, g_pos.first, g_pos.second + off + size);
		g_pos.first.remove_imp(g_pos.second + off, size);
	}
}

}