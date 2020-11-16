#include <cstdint>
#include <glm/mat4x4.hpp>

namespace Rosee {

using cmp_id = size_t;

struct Id
{
	static inline constexpr cmp_id id = 0;
	static void init(void *ptr, size_t size);
	static void destr(void *ptr, size_t size);

	uint32_t value;
};

struct Transform
{
	static inline constexpr cmp_id id = 1;
	static void init(void *ptr, size_t size);
	static void destr(void *ptr, size_t size);

	glm::mat4 mat;
};

namespace Cmp {

using last = Transform;	// keep that updated
static inline constexpr cmp_id max = last::id + 1;
extern size_t size[max];

using init_t = void (void *ptr, size_t size);
using destr_t = void (void *ptr, size_t size);
extern init_t *init[max];
extern destr_t *destr[max];

}
}