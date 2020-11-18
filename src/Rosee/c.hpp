#pragma once

namespace Rosee {

template <typename T, size_t Size>
constexpr size_t array_size(T (&)[Size])
{
	return Size;
}

}