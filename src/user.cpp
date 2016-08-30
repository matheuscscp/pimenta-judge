#include "user.hpp"

#include "database.hpp"

using namespace std;

namespace User {

int login(const string& username, const string& password) {
  DB(users);
  Database::Document user(move(users.retrieve("username",username)));
  if (!user.first || user.second("password").str() != password) return 0;
  return user.first;
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
