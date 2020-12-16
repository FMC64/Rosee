#pragma once

#include <glm/vec2.hpp>
#include <cmath>

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

template <typename RndGen, typename Vec3>
static inline Vec3 genDiffuseVector(RndGen &&gen, const Vec3 &up, double n)
{
	auto a0 = std::acos(std::pow(gen.zrand(), 1.0 / (n + 1.0)));
	auto a1 = 2.0 * pi * gen.zrand();
	auto sa0 = std::sin(a0);
	auto ca0 = std::cos(a0);
	auto sa1 = std::sin(a1);
	auto ca1 = std::cos(a1);
	auto base_diffuse = Vec3(sa0 * ca1, sa0 * sa1, ca0);

	auto nx = Vec3(-up.z, up.x, up.y);
	auto ny = Vec3(up.x, -up.z, up.y);
	auto nz = up;

	return base_diffuse.x * nx + base_diffuse.y * ny + base_diffuse.z * nz;
}

}