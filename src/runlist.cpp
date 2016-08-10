#include <algorithm>

#include "runlist.hpp"

#include "helper.hpp"
#include "global.hpp"
#include "attempt.hpp"

using namespace std;

static map<string,string> runlist;
static pthread_mutex_t runlist_mutex = PTHREAD_MUTEX_INITIALIZER;

namespace Runlist {

void update(map<int,JSON>& attempts) {
  // get necessary settings
  int blind;
  bool blinded;
  {
    time_t beg = Global::settings("contest","begin");
    time_t end = Global::settings("contest","end");
    time_t bld = Global::settings("contest","blind");
    blind = (bld-beg)/60;
    blinded = (bld < end);
  }
  JSON langs = move(Global::settings("contest","languages"));
  
  // convert to struct and sort
  vector<Attempt> atts;
  for (auto& kv : attempts) atts.emplace_back(kv.second);
  sort(atts.begin(),atts.end());
  
  // compute jsons
  map<string,set<string>> ACs;
  map<string,JSON> buf;
  for (auto& att : atts) {
    // answer
    string answer = verdict_tos(att.verdict);
    if (blind <= att.when && blinded) answer = "blind";
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
      {"id"      , att.id},
      {"problem" , att.problem},
      {"time"    , att.when},
      {"language", langs(att.language,"name")},
      {"answer"  , answer}
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
