#include <iostream>
#include "Rosee/Brush.hpp"

using namespace Rosee;

int main() {
        auto b = Brush(Cmp::make_id_array<Id, Transform>());
        auto ent = b.add(64000);
        auto ids = b.get<Id>();
        std::cout << "ent id: " << ent << ", id storage: " << ids << std::endl;
        b.remove(ent, 64000);
        return 0;
}