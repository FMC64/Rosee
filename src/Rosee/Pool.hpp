#pragma once

namespace Rosee {

template <typename T>
struct Pool
{
	T *data;
	size_t size = 0;

	Pool(size_t capacity) :
		data(reinterpret_cast<T*>(std::malloc(capacity * sizeof(T))))
	{
	}
	~Pool(void)
	{
		std::free(data);
	}

	size_t currentIndex(void) const
	{
		return size;
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

	template <typename Destroyer>
	void destroyUsing(Destroyer &&destroyer)
	{
		for (size_t i = 0; i < size; i++)
			destroyer.destroy(data[i]);
	}
};

}