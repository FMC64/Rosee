#include <iostream>
#include "Rosee/Map.hpp"
#include "Rosee/Renderer.hpp"
#include <chrono>

using namespace Rosee;

int main() {
	auto r = Renderer(3, true, false);
	auto m = Map();

	while (true) {
		r.pollEvents();
		if (r.shouldClose())
			break;
		r.render();
	}
	return 0;
}