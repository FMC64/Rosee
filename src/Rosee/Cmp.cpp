#include <glm/mat4x4.hpp>
#include "Cmp.hpp"

namespace Rosee {

void Id::init(void*, size_t)
{
}

void Id::destr(void*, size_t)
{
}

void Transform::init(void*, size_t)
{
}

void Transform::destr(void*, size_t)
{
}

namespace Cmp {

size_t size[max] = {
	sizeof(Id),		// 0
	sizeof(Transform)	// 1
};

init_t *init[max] = {
	Id::init,		// 0
	Transform::init		// 1
};

destr_t *destr[max] = {
	Id::destr,		// 0
	Transform::destr	// 1
};

}
}