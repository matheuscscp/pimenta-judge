#include <dirent.h>
#include <sys/stat.h>

#include "problem.hpp"

#include "helper.hpp"
#include "contest.hpp"
#include "language.hpp"

using namespace std;

namespace Problem {

JSON get_short(int id, int user) {
  DB(problems);
  JSON ans;
  if (
    !problems.retrieve(id,ans) ||
    ans("enabled").isfalse() ||
    !Contest::allow_problem(ans,user)
  ) return JSON::null();
  ans["id"] = id;
  ans.erase("languages");
  return ans;
}

JSON get(int id, int user) {
  JSON ans(move(get_short(id,user)));
  if (!ans) return ans;
  ans["languages"] = Language::list(id);
  ans["has_statement"].setfalse();
  if (statement(id,user) != "") ans["has_statement"].settrue();
  return ans;
}

string statement(int id, int user) {
  JSON tmp(move(get_short(id,user)));
  if (!tmp) return "";
  DIR* dir = opendir(("problems/"+tostr(id)).c_str());
  if (!dir) return "";
  for (dirent* ent = readdir(dir); ent; ent = readdir(dir)) {
    string fn = "problems/"+tostr(id)+"/"+ent->d_name;
    struct stat stt;
    stat(fn.c_str(),&stt);
    if (S_ISREG(stt.st_mode)) { closedir(dir); return fn; }
  }
  closedir(dir);
  return "";
}

JSON page(int user, unsigned p, unsigned ps) {
  DB(problems);
  JSON tmp = problems.retrieve(), ans(vector<JSON>{}), aux;
  for (auto& p : tmp.arr()) {
    if (p("enabled").isfalse()) continue;
    int cid;
    if (p("contest").read(cid)) {
      aux = Contest::get(cid,user);
      if (!aux || !aux("finished")) continue;
    }
    p.erase("languages");
    ans.push_back(move(p));
  }
  if (!ps) {
    p = 0;
    ps = ans.size();
  }
  auto& a = ans.arr();
  unsigned r = (p+1)*ps;
  if (r < a.size()) a.erase(a.begin()+r,a.end());
  a.erase(a.begin(),a.begin()+(p*ps));
  return ans;
}

} // namespace Problem
