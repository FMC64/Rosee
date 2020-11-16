#pragma once

#include "Cmp.hpp"
#include "vector.hpp"
#include "array.hpp"

namespace Rosee {

class Brush
{
	vector<cmp_id> m_cmp_ids;
	void *m_comps[Cmp::max];
	size_t m_size = 0;
	size_t m_allocated = 0;

public:
	Brush(array<cmp_id> cmps);
	~Brush(void);

	size_t size(void) const
	{
		return m_size;
	}

	template <typename Component>
	Component* get(void)
	{
		return reinterpret_cast<Component*>(m_comps[Component::id]);
	}

	size_t add(size_t count);
	void remove(size_t offset, size_t count);
};

}