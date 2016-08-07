#include <algorithm>

#include "runlist.hpp"

#include "helper.hpp"
#include "global.hpp"
#include "attempt.hpp"

using namespace std;

static map<string,string> runlist;
static pthread_mutex_t runlist_mutex = PTHREAD_MUTEX_INITIALIZER;

namespace Runlist {

void update(JSON& attempts) {
  // get necessary settings
  int blind;
  {
    time_t beg = Global::settings("contest","begin");
    time_t bld = Global::settings("contest","blind");
    blind = (bld-beg)/60;
  }
  
  // convert to struct and sort
  vector<Attempt> atts;
  for (auto& kv : attempts.obj())atts.emplace_back(to<int>(kv.first),kv.second);
  sort(atts.begin(),atts.end());
  
  // compute jsons
  map<string,set<string>> ACs;
  map<string,JSON> buf;
  for (auto& att : atts) {
    // answer
    string answer = verdict_tos(att.verdict);
    if (blind <= att.when) answer = "blind";
    else if (!att.judged) answer = "tojudge";
    else if (att.verdict == AC) {
      auto& S = ACs[att.username];
      if (S.find(att.problem) == S.end()) {
        answer = "first";
        S.insert(att.problem);
      }
    }
    // json
    JSON& useratts = buf.emplace(att.username,vector<JSON>({})).first->second;
    useratts.emplace_back(map<string,JSON>{
      {"id"     , att.id},
      {"problem", att.problem},
      {"time"   , att.when},
      {"answer" , answer}
    });
  }
  
  // update runlist
  pthread_mutex_lock(&runlist_mutex);
  runlist.clear();
  for (auto& kv : buf) runlist[kv.first] = kv.second.generate();
  pthread_mutex_unlock(&runlist_mutex);
}

string query(const string& username) {
  pthread_mutex_lock(&runlist_mutex);
  string ans(runlist[username]);
  pthread_mutex_unlock(&runlist_mutex);
  return ans;
}

} // namespace Runlist
