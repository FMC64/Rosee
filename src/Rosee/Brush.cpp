#include "Brush.hpp"

namespace Rosee {

Brush::Brush(array<cmp_id> cmps)
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
	return res;
}

void Brush::remove(size_t offset, size_t count)
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
}

}