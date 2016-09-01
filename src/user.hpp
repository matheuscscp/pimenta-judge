#ifndef USER_H
#define USER_H

#include "json.hpp"

namespace User {

int login(const std::string& username, const std::string& password); // id
JSON get(int id);
std::string name(int id);
JSON profile(int id, int user, unsigned page = 0, unsigned page_size = 0);
JSON page(int user, unsigned page = 0, unsigned page_size = 0);

} // namespace User

#endif
