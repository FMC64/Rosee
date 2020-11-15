#include <cstdint>
#include <glm/mat4x4.hpp>

namespace Rosee {

using cmp_id = size_t;

struct Id
{
	static inline constexpr cmp_id id = 0;

	uint32_t value;
};

struct Transform
{
	static inline constexpr cmp_id id = 1;

	glm::mat4 mat;
};

namespace Cmp {

static inline constexpr cmp_id max = 2;
extern size_t size[max];

}
}