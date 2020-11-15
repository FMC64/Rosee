#include <optional>
#include "Cmp.hpp"
#include "vector.hpp"

namespace Rosee {

class Brush
{
	std::optional<vector<char>> m_comps[static_cast<size_t>(Cmp::Max)];

public:
	Brush(void);
	~Brush(void);
};

}