#pragma once

#include "Static.hpp"
#include <map>

namespace Rosee {
namespace Static {

template <typename Key, typename Value>
auto& get(map<Key, Value> &map)
{
	return reinterpret_cast<std::map<Key, Value>&>(map);
}

template <typename Key, typename Value>
inline void map<Key, Value>::init(void *data)
{
	new (data) std::map<Key, Value>();
}
template <typename Key, typename Value>
inline void map<Key, Value>::destr(void *data)
{
	using T = std::map<Key, Value>;
	reinterpret_cast<T*>(data)->~T();
}

}
}