#pragma once

namespace Rosee {
namespace Static {

template <typename Key, typename Value>
class map
{
	char data[48];

	static void init(void *data);
	static void destr(void *data);

public:
	map(void)
	{
		init(data);
	}
	~map(void)
	{
		destr(data);
	}
};

}
}