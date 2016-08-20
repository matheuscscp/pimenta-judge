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
std::string allow_attempt(time_t when, int userid, int probid);

} // namespace Contest

#endif
