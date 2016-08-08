#ifndef JUDGE_H
#define JUDGE_H

#include "attempt.hpp"

namespace Judge {

void* thread(void*);
std::string attempt(const std::string&,const std::vector<uint8_t>&,Attempt*);

} // namespace Judge

#endif
