#ifndef CLARIFICATION_H
#define CLARIFICATION_H

#include "json.hpp"

namespace Clarification {

JSON query(const std::string&);
std::string question(const std::string&,const std::string&,const std::string&);

} // namespace Clarification

#endif
