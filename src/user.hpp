#ifndef USER_H
#define USER_H

#include "json.hpp"

namespace User {

int login(const std::string& username, const std::string& password); // id
JSON get(int id);
std::string name(int id);

} // namespace User

#endif
