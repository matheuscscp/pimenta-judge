#include <dirent.h>
#include <sys/stat.h>

#include "problem.hpp"

#include "helper.hpp"
#include "contest.hpp"

using namespace std;

namespace Problem {

JSON page(unsigned p, unsigned ps) {
  DB(problems);
  auto probs = move(problems.retrieve([](const Database::Document& problem) {
    if (!problem.second("enabled")) return false;
    return Contest::allow_list_problem(problem);
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

JSON get(int id) {
  DB(problems);
  JSON ans;
  if (!problems.retrieve(id,ans) || !Contest::allow_problem(ans)) return JSON();
  ans["id"] = id;
  return ans;
}

string statement(int id) {
  DB(problems);
  JSON tmp;
  if (!problems.retrieve(id,tmp) || !Contest::allow_problem(tmp)) return "";
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

} // namespace Problem
