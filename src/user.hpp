#ifndef USER_H
#define USER_H

#include <string>

namespace User {

struct Data {
  int id;
  std::string username;
  std::string fullname;
};

Data login(const std::string& username, const std::string& password);

} // namespace User

#endif
