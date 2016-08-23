#ifndef PROBLEM_H
#define PROBLEM_H

#include "json.hpp"

namespace Problem {

JSON get_short(int id);
JSON get(int id);
std::string statement(int id);
JSON page(unsigned page = 0, unsigned page_size = 0);

} // namespace Problem

#endif
