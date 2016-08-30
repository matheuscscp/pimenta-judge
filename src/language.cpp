#include "language.hpp"

#include "database.hpp"

using namespace std;

static JSON list() {
  DB(languages);
  JSON tmp = languages.retrieve();
  JSON ans;
  for (auto& l : tmp.arr()) ans[l["extension"].str()] = l;
  return ans;
}

static void overwrite(map<string,JSON>& dst, map<string,JSON>& src) {
  for (auto& kv : src) dst[kv.first] = move(kv.second);
}

static void overwrite_list(map<string,JSON>& dst, map<string,JSON>& src) {
  for (auto& kv : src) {
    auto it = dst.find(kv.first);
    if (it == dst.end()) dst[kv.first] = move(kv.second);
    else overwrite(it->second.obj(),kv.second.obj());
  }
}

namespace Language {

JSON list(int probid) {
  JSON ans(vector<JSON>{});
  // fetch problem
  DB(problems);
  JSON problem;
  if (!problems.retrieve(probid,problem)) return ans;
  // fetch languages
  JSON languages = ::list();
  auto& langs = languages.obj();
  // fetch languages of problem's contest
  JSON autojudge;
  autojudge.settrue();
  bool contest_finished = true;
  if (problem("contest")) {
    DB(contests);
    JSON contest;
    if (contests.retrieve(int(problem["contest"]),contest)) {
      if (contest("autojudge").isfalse()) autojudge.setfalse();
      if (!contest("finished")) contest_finished = false;
      overwrite_list(langs,contest["languages"].obj());
    }
  }
  // fetch problem's languages
  overwrite_list(langs,problem["languages"].obj());
  // answer
  for (auto& kv : langs) {
    kv.second["id"] = kv.first;
    if (!kv.second("name")) kv.second["name"] = "";
    if (!kv.second("timelimit")) {
      if (!problem("timelimit")) kv.second["timelimit"] = 1;
      else kv.second["timelimit"] = problem["timelimit"];
    }
    if (!kv.second("memlimit")) {
      if (!problem("memlimit")) kv.second["memlimit"] = 1000000;
      else kv.second["memlimit"] = problem["memlimit"];
    }
    if (!kv.second("info")) kv.second["info"] = "";
    if (!kv.second("autojudge").isfalse()) kv.second["autojudge"] = autojudge;
    if (contest_finished) kv.second["autojudge"].settrue();
    if (!kv.second("enabled").isfalse()) ans.push_back(move(kv.second));
  }
  return ans;
}

JSON settings(const JSON& attempt) {
  JSON tmp = move(list(attempt("problem")));
  for (auto& lang : tmp.arr()) {
    if (lang["id"].str() == attempt("language").str()) return lang;
  }
  return JSON::null();
}

} // namespace Language
