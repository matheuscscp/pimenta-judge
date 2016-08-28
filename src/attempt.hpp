#ifndef ATTEMPT_H
#define ATTEMPT_H

#include "json.hpp"

namespace Attempt {

std::string create(JSON&& att, const std::vector<uint8_t>& src);
JSON page(int user, unsigned page = 0, unsigned page_size = 0, int contest = 0);

} // namespace Attempt

#endif
