#pragma once

#include <utility>
#include <new>
#ifdef ROSEE_STANDARD_ITERATOR
#include <iterator>
#endif
#include <type_traits>
#include <cstdlib>
#include <cstring>

namespace Rosee {

template <typename T>
class vector
{
	size_t m_size = 0;
	size_t m_allocated = 0;
	T *m_buf = nullptr;

	void resolve_alloc(void)
	{
		if (m_size > m_allocated) {
			if (m_allocated == 0)
				m_allocated = sizeof(T);
			else
				m_allocated <<= 1;
			m_buf = reinterpret_cast<T*>(std::realloc(m_buf, m_allocated));
		}
	}

public:
	vector(void) = default;
	template <typename ...Args>
	vector(size_t size, Args &&...args)
	{
		resize(size, std::forward<Args>(args)...);
	}
	vector(std::initializer_list<T> init)
	{
		m_size = init.size();
		resolve_alloc();
		size_t ndx = 0;
		for (auto &v : init)
			m_buf[ndx++] = v;
	}
	~vector(void)
	{
		if (m_buf != nullptr) {
			for (size_t i = 0; i < m_size; i++)
				m_buf[i].~T();
			std::free(m_buf);
		}
	}
	vector(vector &&other) :
		m_size(other.m_size),
		m_allocated(other.m_allocated),
		m_buf(other.m_buf)
	{
		other.m_buf = nullptr;
	}
	vector& operator=(vector &&other)
	{
		m_size = other.m_size;
		m_allocated = other.m_allocated;
		m_buf = other.m_buf;

		other.m_buf = nullptr;
	}

	template <typename ...Args>
	T& emplace(Args &&...args)
	{
		size_t cur = m_size++;
		resolve_alloc();
		return *new (&m_buf[cur]) T(std::forward<Args>(args)...);
	}

	T& operator[](size_t ndx) { return m_buf[ndx]; }
	T& operator[](size_t ndx) const { return m_buf[ndx]; }

	class iterator
	{
		vector *m_v;
		size_t m_p;

	public:
#ifdef ROSEE_STANDARD_ITERATOR
		using iterator_category = std::random_access_iterator_tag;
#endif
		using value_type = T;
		using difference_type = int64_t;
		using pointer = T*;
		using reference = T&;

		friend class vector;

		iterator(vector *v, size_t p) :
			m_v(v),
			m_p(p)
		{
		}

		T& operator*(void) const { return (*m_v)[m_p]; }
		T* operator->(void) const { return &(*m_v)[m_p]; }

		bool operator==(const iterator &other) const { return m_p == other.m_p; }
		bool operator!=(const iterator &other) const { return m_p != other.m_p; }
		iterator operator+(size_t off) const { return iterator(m_v, m_p + off); }
		iterator operator-(size_t off) const { return iterator(m_v, m_p - off); }
		iterator& operator++(void) { m_p++; return *this; }
		iterator& operator--(void) { m_p--; return *this; }

		int64_t operator+(const iterator &other) const { return m_p + other.m_p; }
		int64_t operator-(const iterator &other) const { return m_p - other.m_p; }
	};

	friend class iterator;

	iterator begin(void)
	{
		return iterator(this, 0);
	}
	iterator end(void)
	{
		return iterator(this, m_size);
	}

	void erase(const iterator &it)
	{
		it->~T();
		std::memmove(&*it, &*(it + 1), (m_size - it.m_p) * sizeof(T));
		m_size--;
	}

	void erase(const iterator &b, const iterator &e)
	{
		for (size_t i = b.m_p; i < e.m_p; i++)
			m_buf[i].~T();
		std::memmove(&*b, &*e, (m_size - e.m_p) * sizeof(T));
		m_size -= e - b;
	}

private:
	void insert_data(const iterator &b, const T *val, size_t size)
	{
		size_t had_left = m_size - b.m_p;
		m_size += size;
		resolve_alloc();
		std::memmove(&*(b + size), &*b, had_left * sizeof(T));
		std::memcpy(&*b, val, size * sizeof(T));
	}

public:
	template <typename Ob, typename Oe>
	void insert(const iterator &b, Ob &&ob, Oe &&oe)
	{
		insert_data(b, &*ob, oe - ob);
	}

	template <typename It>
	void insert(const iterator &p, It &&to_ins)
	{
		if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<It>>, T>)
			insert_data(p, &to_ins, 1);
		else
			insert_data(p, &*to_ins, 1);
	}

	size_t size(void) const { return m_size; }
	const T* data(void) const { return m_buf; }
	T* data(void) { return m_buf; }
	void reserve(size_t size)
	{
		if (m_allocated < size) {
			m_allocated = size;
			m_buf = reinterpret_cast<T*>(std::realloc(m_buf, m_allocated));
		}
	}

	template <typename ...Args>
	void resize(size_t size, Args &&...args)
	{
		size_t was_size = m_size;
		m_size = size;
		resolve_alloc();
		for (size_t i = was_size; i < size; i++)
			new (&m_buf[i]) T(std::forward<Args>(args)...);
	}
};

}