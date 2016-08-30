#ifndef CONTEST_H
#define CONTEST_H

#include "database.hpp"

namespace Contest {

void fix();

struct Time {
  time_t begin,end,freeze,blind;
};
Time time(const JSON& contest);
time_t begin(const JSON& contest);
time_t end(const JSON& contest);
time_t freeze(const JSON& contest);
time_t blind(const JSON& contest);

bool allow_problem(const JSON& problem, int user);
bool allow_create_attempt(JSON& attempt, const JSON& problem);

JSON get(int id, int user);
JSON get_problems(int id, int user);
JSON get_attempts(int id, int user);
JSON scoreboard(int id, int user);
JSON page(unsigned page = 0, unsigned page_size = 0);

} // namespace Contest

#endif
