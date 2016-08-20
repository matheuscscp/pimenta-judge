#ifndef ATTEMPT_H
#define ATTEMPT_H

#include "json.hpp"

namespace Attempt {

std::string create(JSON&& att, const std::vector<uint8_t>& src);

} // namespace Attempt

#endif
