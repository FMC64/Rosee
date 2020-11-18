#pragma once

namespace Rosee {

using cmp_id = size_t;

namespace Cmp {

using init_fun_t = void (void *ptr, size_t size);
using destr_fun_t = void (void *ptr, size_t size);

using init_t = init_fun_t*;
using destr_t = destr_fun_t*;

}

}