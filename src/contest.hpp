#ifndef CONTEST_H
#define CONTEST_H

#include "database.hpp"

namespace Contest {

struct Time {
  time_t begin,end,freeze,blind;
};
Time time(const JSON& contest);
time_t begin(const JSON& contest);
time_t end(const JSON& contest);
time_t freeze(const JSON& contest);
time_t blind(const JSON& contest);

bool allow_list_problem(const Database::Document& problem);
bool allow_problem(const JSON& problem);
bool allow_create_attempt(JSON& attempt, const JSON& problem);

JSON get(int id);
JSON page(unsigned page = 0, unsigned page_size = 0);

} // namespace Contest

#endif
