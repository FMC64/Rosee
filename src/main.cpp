#include <iostream>
#include <algorithm>
#include "Rosee/vector.hpp"

int main(void)
{
	/*Rosee::vector<size_t> vec {1, 2, 3};

	vec.emplace(5);
	vec.emplace(8);
	vec.emplace(1);
	vec.emplace(2);
	vec.emplace(3);*/
	/*vec.erase(std::remove_if(vec.begin(), vec.end(), [](auto &v){
		return v >= 3;
	}), vec.end());*/
	//vec.erase(vec.begin());
	//vec.insert(vec.begin() + 1, 5ULL);

	auto vec = Rosee::vector<size_t>(4, 58);
	for (auto &v : vec)
		std::cout << v << std::endl;
	return 0;
}