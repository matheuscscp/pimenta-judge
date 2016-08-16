#include "user.hpp"

#include "database.hpp"

using namespace std;

namespace User {

string login(const string& username, const string& password) {
  Database::Collection users("users");
  Database::Document user(move(users.retrieve("username",username)));
  if (!user.first || user.second("password").str() != password) return "";
  return user.second("fullname").str();
}

} // namespace User
