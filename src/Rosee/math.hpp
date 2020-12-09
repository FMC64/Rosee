#pragma once

#include <glm/vec2.hpp>

namespace Rosee {

template <typename T>
const T& min(const T &a, const T &b)
{
	return a < b ? a : b;
}

template <typename T>
const T& max(const T &a, const T &b)
{
	return a > b ? a : b;
}

template <typename T>
T clamp(const T &val, const T &mn, const T &mx)
{
	return min(max(val, mn), mx);
}

using svec2 = glm::vec<2, size_t>;
static inline constexpr double pi = 3.141592653589793238462643383279502884;

}