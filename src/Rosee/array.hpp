#pragma once

#include <cstdint>
#include <utility>

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
		data(const_cast<T*>(container.data()))
	{
	}
};

template <typename T, size_t Size>
class sarray
{
	T m_data[Size];

public:
	constexpr size_t size(void) const
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

	constexpr T* cdata(void)
	{
		return m_data;
	}

private:
	constexpr void swap(T& l, T& r)
	{
		T tmp = std::move(l);
		l = std::move(r);
		r = std::move(tmp);
	}

	constexpr void sort_impl(size_t left, size_t right)
	{
		if (left < right) {
			size_t m = left;

			for (size_t i = left + 1; i < right; i++)
				if (m_data[i] < m_data[left])
					swap(m_data[++m], m_data[i]);

			swap(m_data[left], m_data[m]);

			sort_impl(left, m);
			sort_impl(m + 1, right);
		}
	}

public:
	constexpr void sort(void)
	{
		sort_impl(0, Size);
	}
};

}