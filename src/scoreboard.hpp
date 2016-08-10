#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include "json.hpp"

namespace Scoreboard {

void update(std::map<int,JSON>&);
std::string query();

} // namespace Scoreboard

#endif
