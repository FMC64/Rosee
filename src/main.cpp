#include <iostream>
#include "Rosee/Map.hpp"
#include <chrono>

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
	auto ent = m.add<Id, Transform>(64000000);
	m.add<Id, Transform>(129);
	std::cout << "ent id: " << ent << std::endl;

	{
		auto [b, n] = m.find(ent);
		auto trans = b->get<Transform>();
		auto bef = std::chrono::high_resolution_clock::now();
		std::memset(trans, 0, 64000000 * sizeof(glm::mat4));
		auto now = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> dif = now - bef;
		std::cout << "set: " << dif.count() << std::endl;
		std::memset(trans, 0, 64000000 * sizeof(glm::mat4));
		auto now2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> dif2 = now2 - now;
		std::cout << "set2: " << dif2.count() << std::endl;
	}
	auto bef = std::chrono::high_resolution_clock::now();
	m.remove(ent + 2, 100);
	auto now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> dif = now - bef;
	std::cout << "remove: " << dif.count() << std::endl;
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