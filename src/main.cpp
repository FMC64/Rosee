#include <iostream>
#include "Rosee/Map.hpp"

using namespace Rosee;

int main() {
	auto m = Map();
	auto [b, ent] = m.add<Id, Transform>(64000);
	auto ids = b.get<Id>();
	std::cout << "ent id: " << ent << ", id storage: " << ids << std::endl;
	b.remove(ent, 64000);
	return 0;
}