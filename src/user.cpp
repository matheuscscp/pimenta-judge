#include "user.hpp"

#include "database.hpp"

using namespace std;

namespace User {

Data login(const string& username, const string& password) {
  Data ans;
  ans.id = 0;
  Database::Collection users("users");
  Database::Document user(move(users.retrieve("username",username)));
  if (!user.first || user.second("password").str() != password) return ans;
  ans.id = user.first;
  ans.username = username;
  ans.fullname = user.second("fullname").str();
  return ans;
}

} // namespace User
