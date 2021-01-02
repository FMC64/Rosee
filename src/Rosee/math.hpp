#pragma once

#include <glm/vec2.hpp>
#include <glm/gtx/transform.hpp>
#include <cmath>
#include <tuple>

namespace Rosee {

template <typename T>
T min(const T &a, const T &b)
{
	return a < b ? a : b;
}

template <typename T>
T max(const T &a, const T &b)
{
	return a > b ? a : b;
}

template <typename T>
T abs(const T &a)
{
	return a >= static_cast<T>(0) ? a : -a;
}

template <typename T>
T clamp(const T &val, const T &mn, const T &mx)
{
	return min(max(val, mn), mx);
}

using svec2 = glm::vec<2, size_t>;
using ivec2 = glm::vec<2, int64_t>;
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

	Vec3 nx;
	Vec3 ny;
	if (std::fabs(up.x) > std::fabs(up.y))
		nx = Vec3(up.z, 0, -up.x) / sqrtf(up.x * up.x + up.z * up.z);
	else
		nx = Vec3(0, -up.z, up.y) / sqrtf(up.y * up.y + up.z * up.z);
	ny = glm::cross(up, nx);
	auto nz = up;

	return base_diffuse.x * nx + base_diffuse.y * ny + base_diffuse.z * nz;
}

static inline std::tuple<uint64_t, bool> iabs_save_sign(int64_t value)
{
	if (value < 0)
		return std::tuple<uint64_t, bool>(-value, true);
	else
		return std::tuple<uint64_t, bool>(value, false);
}

static inline int64_t iabs_restore_sign(uint64_t value, bool is_neg)
{
	if (is_neg)
		return -value;
	else
		return value;
}

}