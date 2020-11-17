#include <stdexcept>
#include "Brush.hpp"
#include "Map.hpp"
#include "StaticMap.hpp"

namespace Rosee {

Brush::Brush(Map &map, array<cmp_id> cmps) :
	m_map(map)
{
	m_cmp_ids.resize(cmps.size);
	std::memcpy(m_cmp_ids.data(), cmps.data, cmps.size * sizeof(cmp_id));
	std::memset(m_comps, 0, sizeof(m_comps));
	std::memset(m_cmp_pres, 0, Cmp::max);
	for (auto &c : m_cmp_ids)
		m_cmp_pres[c] = true;
}

Brush::~Brush(void)
{
	for (auto &c : m_cmp_ids) {
		auto data = m_comps[c];
		Cmp::destr[c](data, m_size);
		std::free(data);
	}
}

size_t Brush::add(size_t count)
{
	size_t res = m_size;
	m_size += count;

	if (m_size > m_allocated) {
		if (m_allocated == 0)
			m_allocated = 1;
		while (m_size > m_allocated)
			m_allocated <<= 1;
		for (auto &c : m_cmp_ids)
			m_comps[c] = std::realloc(m_comps[c], m_allocated * Cmp::size[c]);
	}
	for (auto &c : m_cmp_ids)
		Cmp::init[c](reinterpret_cast<char*>(m_comps[c]) + res * Cmp::size[c], count);
	if (cmpIsPres<Id>()) {
		auto ids = get<Id>() + res;
		size_t base_id = m_map.m_id;
		size_t acc_id = base_id;
		m_map.m_id += count;
		for (size_t i = 0; i < count; i++)
			ids[i] = acc_id++;
		m_map.add_range(base_id, base_id + count, *this, res);
	}
	return res;
}

void Brush::remove_imp(size_t offset, size_t count)
{
	size_t end = offset + count;
	size_t rest = m_size - end;
	for (auto &c : m_cmp_ids) {
		auto cptr = reinterpret_cast<char*>(m_comps[c]);
		size_t csize = Cmp::size[c];
		Cmp::destr[c](cptr + offset * csize, count);
		std::memmove(cptr + offset * csize, cptr + end * csize, rest * csize);
	}
	m_size -= count;
	if (cmpIsPres<Id>()) {
		auto ids = get<Id>();
		auto &m = Static::get(m_map.m_ids);
		size_t cur = offset;
		while (cur < m_size) {
			auto got = m.find(Map::Range{ids[cur], ids[cur] + 1});
			if (got == m.end())
				throw std::runtime_error("Can't find range to offset");
			got->second.second -= count;
			cur += got->first.end - got->first.begin;
		}
	}
}

void Brush::remove(size_t offset, size_t count)
{
	if (cmpIsPres<Id>()) {
		auto ids = get<Id>();
		auto &m = Static::get(m_map.m_ids);
		while (count > 0) {
			auto got = m.find(Map::Range{ids[offset], ids[offset] + 1});
			if (got == m.end())
				throw std::runtime_error("Can't find id range to remove");
			size_t len = got->first.end - got->first.begin;
			m_map.remove(got->first.begin, len);
			count -= len;
		}
	} else
		remove_imp(offset, count);
}

}