#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#include "json.hpp"

namespace Scoreboard {

void update(JSON&);
std::string query();

} // namespace Scoreboard

#endif
