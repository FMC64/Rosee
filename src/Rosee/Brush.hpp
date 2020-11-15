#include <optional>
#include "Cmp.hpp"
#include "vector.hpp"
#include "array.hpp"

namespace Rosee {

class Brush
{
	vector<cmp_id> m_cmp_ids;
	vector<char> m_comps[Cmp::max];
	size_t m_size = 0;

public:
	Brush(array<cmp_id> cmps);

	size_t size(void) const
	{
		return m_size;
	}

	template <typename Component>
	Component* get(void)
	{
		return reinterpret_cast<Component*>(m_comps[Component::id].data());
	}

	size_t add(void);
};

}