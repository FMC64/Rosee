#pragma once

#include <cstdint>

namespace Rosee {

template <typename T>
class array
{
public:
	size_t size;
	T *data;

	template <typename Container>
	array(Container &&container) :
		size(container.size()),
		data(container.data())
	{
	}
};

template <typename T, size_t Size>
class sarray
{
	T m_data[Size];

public:
	sarray(void)
	{
	}

	size_t size(void) const
	{
		return Size;
	}

	T* data(void)
	{
		return m_data;
	}

	const T* data(void) const
	{
		return m_data;
	}
};

}