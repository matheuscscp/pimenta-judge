#ifndef PROBLEM_H
#define PROBLEM_H

#include "json.hpp"

namespace Problem {

JSON page(unsigned page = 0, unsigned page_size = 0);
JSON get(int id);
std::string statement(int id);

} // namespace Problem

#endif
