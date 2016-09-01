#include "user.hpp"

#include "database.hpp"

using namespace std;

namespace User {

int login(const string& username, const string& password) {
  DB(users);
  JSON user = users.retrieve(map<string,JSON>{
    {"username", username},
    {"password", password}
  });
  if (user.size() == 0) return 0;
  return user(0,"id");
}

JSON get(int id) {
  DB(users);
  return users.retrieve(id);
}

string name(int id) {
  JSON tmp = get(id);
  if (!tmp) return "";
  return tmp["name"];
}

} // namespace User
