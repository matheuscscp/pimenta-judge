#include <set>

#include "user.hpp"

#include "cmap.hpp"
#include "helper.hpp"
#include "database.hpp"
#include "attempt.hpp"

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

JSON profile(int id, int user, unsigned p, unsigned ps) {
  JSON tmp = get(id);
  if (!tmp) return tmp;
  cmap<int,string> ans0;
  JSON atts = Attempt::page(user,0,0,0,false,true);
  for (auto& att : atts.arr()) {
    if (
      int(att["user"]) == id &&
      att["status"].str() == "judged" &&
      verdict_toi(att["verdict"]) == AC
    ) ans0[att["problem"]["id"]] = att["problem"]["name"].str();
  }
  if (!ps) {
    p = 0;
    ps = tmp.size();
  }
  JSON ans(map<string,JSON>{{"name",tmp["name"]},{"solved",vector<JSON>{}}});
  if (ans0.size() <= p*ps) return ans;
  auto it = ans0.at(p*ps);
  for (int i = 0; i < ps && it != ans0.end(); i++, it++) {
    ans["solved"].push_back(map<string,JSON>{
      {"id"   , it->first},
      {"name" , it->second}
    });
  }
  return ans;
}

JSON page(int user, unsigned p, unsigned ps) {
  struct stats {
    set<int> solved, tried;
  };
  map<int,stats> info;
  JSON atts = Attempt::page(user,0,0,0,false,true);
  for (auto& att : atts.arr()) {
    auto& us = info[att["user"]];
    us.tried.insert(int(att["problem"]["id"]));
    if (att["status"].str() == "judged" && verdict_toi(att["verdict"]) == AC) {
      us.solved.insert(int(att["problem"]["id"]));
    }
  }
  DB(users);
  JSON tmp = users.retrieve(), ans(vector<JSON>{});
  if (!ps) {
    p = 0;
    ps = tmp.size();
  }
  for (int i = p*ps, j = 0; i < tmp.size() && j < ps; i++, j++) {
    auto& us = tmp[i];
    ans.push_back(map<string,JSON>{
      {"id"     , us["id"]},
      {"name"   , us["name"]},
      {"solved" , info[us["id"]].solved.size()},
      {"tried"  , info[us["id"]].tried.size()}
    });
  }
  return ans;
}

} // namespace User
