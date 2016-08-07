#ifndef RUNLIST_H
#define RUNLIST_H

#include "json.hpp"

namespace Runlist {

void update(JSON&);
std::string query(const std::string&);

} // namespace Runlist

#endif
