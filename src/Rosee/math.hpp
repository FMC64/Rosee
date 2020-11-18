#pragma once

#include <glm/vec2.hpp>

namespace Rosee {

template <typename T>
const T& min(const T &a, const T &b)
{
	return a < b ? a : b;
}

template <typename T>
T& max(const T &a, const T &b)
{
	return a > b ? a : b;
}

using svec2 = glm::vec<2, size_t>;

}