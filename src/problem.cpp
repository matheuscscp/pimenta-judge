#include <dirent.h>
#include <sys/stat.h>

#include "problem.hpp"

#include "helper.hpp"
#include "contest.hpp"
#include "language.hpp"

using namespace std;

namespace Problem {

JSON get_short(int id) {
  DB(problems);
  JSON ans;
  if (
    !problems.retrieve(id,ans) ||
    ans("enabled").isfalse() ||
    !Contest::allow_problem(ans)
  ) return JSON::null();
  ans["id"] = id;
  ans.erase("languages");
  return ans;
}

JSON get(int id) {
  JSON ans(move(get_short(id)));
  if (!ans) return ans;
  ans["languages"] = Language::list(id);
  ans["has_statement"].setfalse();
  if (statement(id) != "") ans["has_statement"].settrue();
  return ans;
}

string statement(int id) {
  JSON tmp(move(get_short(id)));
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

JSON page(unsigned p, unsigned ps, int contest) {
  DB(problems);
  auto probs = move(problems.retrieve([&](const Database::Document& problem) {
    if (
      problem.second("enabled").isfalse() ||
      (contest != -1 && (
        !problem.second("contest") ||
        int(problem.second("contest")) != contest
      )) ||
      (contest == -1 && !Contest::allow_list_problem(problem))
    ) return Database::null();
    Database::Document ans(problem);
    ans.second.erase("languages");
    return ans;
  }));
  if (!ps) {
    p = 0;
    ps = probs.size();
  }
  JSON ans(vector<JSON>{});
  for (int i = p*ps, j = 0; i < probs.size() && j < ps; i++, j++) {
    probs[i].second["id"] = probs[i].first;
    ans.push_back(move(probs[i].second));
  }
  return ans;
}

} // namespace Problem
