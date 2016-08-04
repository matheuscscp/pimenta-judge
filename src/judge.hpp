#ifndef JUDGE_H
#define JUDGE_H

#include "global.hpp"

namespace Judge {

void fire();
std::string attempt(const std::string&, const std::vector<uint8_t>&, Attempt);

} // namespace Judge

#endif
