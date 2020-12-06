#pragma once

#include "Cmp.hpp"
#include "vector.hpp"
#include "array.hpp"

namespace Rosee {

class Map;

class Brush
{
	Map &m_map;
	friend class Map;

	vector<cmp_id> m_cmp_ids;
	bool m_cmp_pres[Cmp::max];
	void *m_comps[Cmp::max];
	size_t m_size = 0;
	size_t m_allocated = 0;

public:
	Brush(Map &map, array<cmp_id> cmps);
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

	template <typename Component>
	Component* get(cmp_id cmp)
	{
		return reinterpret_cast<Component*>(m_comps[cmp]);
	}

	void* get(cmp_id cmp)
	{
		return m_comps[cmp];
	}


	bool cmpIsPres(cmp_id cmp) const
	{
		return m_cmp_pres[cmp];
	}

	template <typename Component>
	bool cmpIsPres(void) const
	{
		return m_cmp_pres[Component::id];
	}

	const vector<cmp_id>& cmpsGet(void) const
	{
		return m_cmp_ids;
	}

	size_t add(size_t count);

private:
	void remove_imp(size_t offset, size_t count);

public:
	void remove(size_t offset, size_t count);
};

}