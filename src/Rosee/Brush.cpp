#include "Brush.hpp"

namespace Rosee {

Brush::Brush(array<cmp_id> cmps)
{
	m_cmp_ids.resize(cmps.size);
	std::memcpy(m_cmp_ids.data(), cmps.data, cmps.size * sizeof(cmp_id));
}

size_t Brush::add(void)
{
	size_t res = m_size++;

	for (auto &c : m_cmp_ids)
		m_comps[c].resize(m_size * Cmp::size[c]);
	return res;
}

}