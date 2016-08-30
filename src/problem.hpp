#ifndef PROBLEM_H
#define PROBLEM_H

#include "json.hpp"

namespace Problem {

JSON get_short(int id, int user);
JSON get(int id, int user);
std::string statement(int id, int user);
JSON page(int user, unsigned page = 0, unsigned page_size = 0);

} // namespace Problem

#endif
