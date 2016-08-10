#include <algorithm>

#include "scoreboard.hpp"

#include "helper.hpp"
#include "global.hpp"
#include "attempt.hpp"

using namespace std;

static string scoreboard;
static pthread_mutex_t scoreboard_mutex = PTHREAD_MUTEX_INITIALIZER;

namespace Scoreboard {

void update(map<int,JSON>& attempts) {
  typedef pair<int,int> problem_t;
  struct Entry {
    string fullname;
    vector<problem_t> problems;
    int solved;
    int penalty;
    vector<int> ACs;
    void compute_score() {
      solved = 0;
      penalty = 0;
      for (const problem_t& p : problems) {
        if (p.first <= 0) continue;
        solved++;
        penalty += (p.second + 20*(p.first-1));
      }
    }
    bool operator<(const Entry& other) const {
      if (solved != other.solved) return solved > other.solved;
      if (penalty != other.penalty) return penalty < other.penalty;
      for (int i = solved-1; 0 <= i; i--) {
        if (ACs[i] != other.ACs[i]) return ACs[i] < other.ACs[i];
      }
      return false;
    }
    JSON json() const {
      JSON ans;
      ans("fullname") = fullname;
      ans("problems") = vector<JSON>{};
      for (auto& p : problems) ans("problems").emplace_back(map<string,JSON>{
        {"cnt" , p.first},
        {"time", p.second}
      });
      ans("solved") = solved;
      ans("penalty") = penalty;
      return ans;
    }
  };
  
  // get necessary settings
  int freeze;
  bool frozen;
  {
    time_t beg = Global::settings("contest","begin");
    time_t end = Global::settings("contest","end");
    time_t frz = Global::settings("contest","freeze");
    time_t bld = Global::settings("contest","blind");
    frz = min(frz,bld);
    freeze = (frz-beg)/60;
    frozen = (frz <= time(nullptr) && frz < end);
  }
  JSON problems(move(Global::settings("contest","problems")));
  JSON users(move(Global::settings("users")));
  
  // convert to struct and sort
  vector<Attempt> atts;
  for (auto& kv : attempts) {
    Attempt att(kv.second);
    if (freeze <= att.when || !att.judged) continue;
    atts.push_back(move(att));
  }
  sort(atts.begin(),atts.end());
  
  // compute entries
  map<string, Entry> entriesmap;
  for (Attempt& att : atts) {
    int prob = problems(att.problem,"index");
    auto it = entriesmap.find(att.username);
    if (it == entriesmap.end()) {
      Entry& entry = entriesmap[att.username];
      entry.fullname = users(att.username,"fullname").str();
      entry.problems = vector<problem_t>(problems.size(),make_pair(0,0));
      if (att.verdict != AC) entry.problems[prob].first = -1;
      else {
        entry.problems[prob].first = 1;
        entry.problems[prob].second = att.when;
        entry.ACs.push_back(att.when);
      }
    }
    else {
      problem_t& p = it->second.problems[prob];
      if (p.first > 0) continue;
      if (att.verdict != AC) p.first--;
      else {
        p.first  = 1 - p.first;
        p.second = att.when;
        it->second.ACs.push_back(att.when);
      }
    }
  }
  
  // sort entries
  vector<Entry> entries;
  for (auto& kv : entriesmap) {
    kv.second.compute_score();
    entries.push_back(kv.second);
  }
  sort(entries.begin(),entries.end());
  
  // compute json
  JSON buf(map<string,JSON>{
    {"frozen"    , frozen ? "true" : "false"},
    {"scoreboard", vector<JSON>{}}
  });
  for (auto& e : entries) buf("scoreboard").push_back(move(e.json()));
  
  // update scoreboard
  pthread_mutex_lock(&scoreboard_mutex);
  scoreboard = buf.generate();
  pthread_mutex_unlock(&scoreboard_mutex);
}

string query() {
  pthread_mutex_lock(&scoreboard_mutex);
  string ans(scoreboard);
  pthread_mutex_unlock(&scoreboard_mutex);
  return ans;
}

} // namespace Scoreboard
