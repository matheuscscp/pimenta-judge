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
  JSON problem = Problem::get_short(att["problem"],att["user"]);
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

JSON get(int id, int user) {
  return JSON::null();
}

JSON page(int user, unsigned p, unsigned ps, int contest) {
  DB(attempts);
  JSON ans(vector<JSON>{}), aux;
  attempts.retrieve([&](const Database::Document& doc) {
    JSON att = doc.second;
    int cid;
    if (!att("contest").read(cid)) { if (contest) return Database::null(); }
    else {
      if (contest) {
        if (contest != cid) return Database::null();
      }
      else {
        aux = Contest::get(cid);
        if (!aux || !aux("finished")) return Database::null();
      }
    }
    int pid = att("problem");
    aux = Problem::get_short(pid,user);
    if (!aux) return Database::null();
    att["id"] = doc.first;
    att["language"] = Language::settings(att)["name"];
    att["problem"] = move(map<string,JSON>{
      {"id"   , pid},
      {"name" , aux["name"]}
    });
    att.erase("ip");
    att.erase("time");
    att.erase("memory");
    if (att["status"] != "judged") att.erase("verdict");
    ans.push_back(move(att));
    return Database::null();
  });
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

} // namespace Attempt
