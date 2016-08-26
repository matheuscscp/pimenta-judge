#include "attempt.hpp"

#include "helper.hpp"
#include "problem.hpp"
#include "language.hpp"
#include "contest.hpp"
#include "database.hpp"
#include "judge.hpp"

using namespace std;

namespace Attempt {

string create(JSON&& att, const vector<uint8_t>& src) {
  JSON problem = Problem::get_short(att["problem"]);
  if (!problem) return "Problem "+att["problem"].str()+" do not exists!";
  JSON setts = Language::settings(att);
  if (!setts) return "Language "+att["language"].str()+" do not exists!";
  if (!Contest::allow_create_attempt(att,problem)) {
    return "Attempts are not allowed right now.";
  }
  DB(attempts);
  int id = attempts.create(att);
  string fn = "attempts/"+tostr(id)+"/";
  system("mkdir -p %soutput",fn.c_str());
  fn += att["problem"].str();
  fn += att["language"].str();
  FILE* fp = fopen(fn.c_str(), "wb");
  fwrite(&src[0],src.size(),1,fp);
  fclose(fp);
  Judge::push(id);
  return "Attempt "+tostr(id)+" received.";
}

JSON page(int user, unsigned p, unsigned ps, int contest) {
  DB(attempts);
  auto atts = move(attempts.retrieve([&](const Database::Document& doc) {
    if (
      user != (int)doc.second("user") ||
      (contest != -1 && (
        !doc.second("contest") ||
        contest != (int)doc.second("contest")
      ))
    ) return Database::null();
    return doc;
  }));
  if (!ps) {
    p = 0;
    ps = atts.size();
  }
  JSON ans(vector<JSON>{});
  for (int i = p*ps, j = 0; i < atts.size() && j < ps; i++, j++) {
    JSON& att = atts[i].second;
    Contest::transform_attempt(att); // change status field to blind?
    att["language"] = Language::settings(att)["name"];
    att["id"] = atts[i].first;
    att.erase("ip");
    att.erase("time");
    att.erase("memory");
    if (att["status"] != "judged") att.erase("verdict");
    int prob = att["problem"];
    att["problem"] = JSON(map<string,JSON>{
      {"id"   , prob},
      {"name" , Problem::get_short(prob)["name"]}
    });
    ans.push_back(move(att));
  }
  return ans;
}

} // namespace Attempt
