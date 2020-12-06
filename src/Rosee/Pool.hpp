#pragma once

namespace Rosee {

template <typename T>
class Pool
{
	T *data;
	size_t size = 0;

public:
	Pool(size_t capacity) :
		data(reinterpret_cast<T*>(std::malloc(capacity * sizeof(T))))
	{
	}
	~Pool(void)
	{
		std::free(data);
	}

	T* allocate(void)
	{
		return &data[size++];
	}

	template <typename ...Args>
	void destroy(Args &&...args)
	{
		for (size_t i = 0; i < size; i++)
			data[i].destroy(std::forward<Args>(args)...);
	}
};

}