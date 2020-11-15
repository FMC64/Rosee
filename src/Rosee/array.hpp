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

}