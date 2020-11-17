#include <iostream>
#include "Rosee/Map.hpp"

using namespace Rosee;

int main() {
	auto m = Map();

	/*auto [b, ent] = m.add<Id, Transform>(64000);
	auto ids = b.get<Id>();
	std::cout << "ent id: " << ent << ", id storage: " << ids << std::endl;
	b.remove(ent, 64000);
	*/

	//for (size_t i = 0; i < 100; i++) {
	m.add<Id, Transform>(73);
	auto ent = m.add<Id, Transform>(64000);
	//m.add<Id, Transform>(129);
	std::cout << "ent id: " << ent << std::endl;
	m.remove(ent + 2, 100);
	{
		auto [b, n] = m.find(ent + 1);
		std::cout << "got b: " << b << ", n: " << n << std::endl;
	}
	{
		auto [b, n] = m.find(ent + 2 + 15);
		std::cout << "got b: " << b << ", n: " << n << std::endl;
	}
	{
		auto [b, n] = m.find(ent + 63999);
		std::cout << "got b: " << b << ", n: " << n << std::endl;
	}
	//}
	/*for (size_t i = 0; i < 100; i++) {
		for (size_t j = 0; j < 64000; j++) {
			auto ent = m.add<Id, Transform>(1);
			auto [b, n] = m.find(ent);
		}
	}*/
	return 0;
}