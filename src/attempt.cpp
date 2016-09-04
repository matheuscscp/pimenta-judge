#include "attempt.hpp"

#include "helper.hpp"
#include "problem.hpp"
#include "language.hpp"
#include "contest.hpp"
#include "database.hpp"
#include "judge.hpp"

using namespace std;

static string source(const string& fn) {
  string ans;
  char* buf = new char[(1<<20)+1];
  FILE* fp = fopen(fn.c_str(),"rb");
  if (!fp) return "";
  for (int sz; (sz = fread(buf,1,1<<20,fp)) > 0; buf[sz] = 0, ans += buf);
  fclose(fp);
  delete[] buf;
  return ans;
}

namespace Attempt {

void fix() {
  DB(attempts);
  DB(contests);
  JSON aux;
  attempts.update([&](Database::Document& doc) {
    auto& j = doc.second.obj();
    if (
      j.find("when") != j.end() ||
      j.find("contest") == j.end() ||
      j.find("contest_time") == j.end() ||
      !contests.retrieve(doc.second["contest"],aux)
    ) {
      return false;
    }
    doc.second["when"] = 60*int(doc.second["contest_time"])+Contest::begin(aux);
    return true;
  });
}

string create(JSON&& att, const vector<uint8_t>& src) {
  // check stuff
  JSON problem = Problem::get_short(att["problem"],att["user"]);
  if (!problem) return "Problem "+att["problem"].str()+" do not exists!";
  JSON setts = Language::settings(att);
  if (!setts) return "Language "+att["language"].str()+" do not exists!";
  if (!Contest::allow_create_attempt(att,problem)) {
    return "Attempts are not allowed right now.";
  }
  // update db
  DB(attempts);
  int id = attempts.create(att);
  // save file
  string fn = "attempts/"+tostr(id)+"/";
  system("mkdir -p %soutput",fn.c_str());
  fn += att["problem"].str();
  fn += att["language"].str();
  FILE* fp = fopen(fn.c_str(), "wb");
  fwrite(&src[0],src.size(),1,fp);
  fclose(fp);
  // push
  Judge::push(id);
  // msg
  return "Attempt "+tostr(id)+" received.";
}

JSON get(int id, int user) {
  DB(attempts);
  JSON ans;
  if (!attempts.retrieve(id,ans)) return JSON::null();
  if (user != int(ans["user"])) return JSON::null();
  int cid;
  if (ans("contest").read(cid) && !Contest::get(cid,user)) return JSON::null();
  int pid = ans["problem"];
  JSON prob = Problem::get_short(pid,user);
  if (!prob) return JSON::null();
  ans["id"] = id;
  string ext = ans["language"];
  ans["language"] = Language::settings(ans)["name"];
  ans["problem"] = move(map<string,JSON>{
    {"id"   , pid},
    {"name" , prob["name"]}
  });
  ans["source"] = source("attempts/"+tostr(id)+"/"+tostr(pid)+ext);
  ans.erase("ip");
  ans.erase("time");
  ans.erase("memory");
  if (ans["status"] != "judged") ans.erase("verdict");
  return ans;
}

JSON page(
  int user,
  unsigned p,
  unsigned ps,
  int contest,
  bool scoreboard,
  bool profile
) {
  DB(attempts);
  JSON tmp = attempts.retrieve(), ans(vector<JSON>{}), aux;
  for (auto& att : tmp.arr()) {
    if (!scoreboard && !profile && int(att["user"]) != user) continue;
    if ((scoreboard || profile) && att("privileged")) continue;
    int cid;
    bool hasc = att("contest").read(cid);
    if (contest) {
      if (!hasc || cid != contest) continue;
    }
    else if (hasc) {
      aux = Contest::get(cid,user);
      if (!aux || !aux("finished")) continue;
    }
    int pid = att["problem"];
    aux = Problem::get_short(pid,user);
    if (!aux) continue;
    att["language"] = Language::settings(att)["name"];
    att["problem"] = move(map<string,JSON>{
      {"id"   , pid},
      {"name" , aux["name"]}
    });
    att.erase("ip");
    att.erase("time");
    att.erase("memory");
    if (att["status"] != "judged") {
      if (scoreboard) continue;
      att.erase("verdict");
    }
    ans.push_back(move(att));
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

} // namespace Attempt
