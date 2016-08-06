#include <map>
#include <algorithm>

#include <unistd.h>

#include "scoreboard.hpp"

#include "global.hpp"

using namespace std;

static string buf1, buf2;
static string* frontbuf = &buf1;
static string* backbuf = &buf2;
static pthread_mutex_t frontbuf_mutex = PTHREAD_MUTEX_INITIALIZER;

static void update() {
  typedef pair<int, time_t> problem_t;
  struct Entry {
    string team;
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
    string str() const {
      string ret;
      ret += ("<td>"+team+"</td>");
      char i = 'A';
      for (const problem_t& p : problems) {
        ret += "<td class=\"problem\">";
        if (p.first > 0) {
          ret += balloon_img(i) + to<string>(p.first)+"/";
          ret += to<string>(p.second);
        }
        else if (p.first < 0) ret += to<string>(-p.first)+"/-";
        ret += "</td>";
        i++;
      }
      ret += ("<td>"+to<string>(solved)+" ("+to<string>(penalty)+")"+"</td>");
      return ret;
    }
  };
  
  // read file
  JSON contest(move(Global::settings("contest")));
  time_t begin = contest("begin");
  time_t freeze = contest("freeze");
  time_t blind = contest("blind");
  auto nproblems = contest("problems").size();
  vector<Attempt> atts; Attempt att;
  Global::lock_att_file();
  FILE* fp = fopen("attempts.txt", "r");
  if (fp) {
    while (att.read(fp)) {
      time_t when = begin + 60*att.when;
      if (freeze <= when || blind <= when) continue;
      if (att.status == "judged") atts.push_back(att);
    }
    fclose(fp);
  }
  Global::unlock_att_file();
  stable_sort(atts.begin(),atts.end());
  
  // compute entries
  map<string, Entry> entriesmap;
  for (Attempt& att : atts) {
    auto it = entriesmap.find(att.teamname);
    if (it == entriesmap.end()) {
      Entry& entry = entriesmap[att.teamname];
      entry.team = att.teamname;
      entry.problems = vector<problem_t>(nproblems,make_pair(0,0));
      if (att.verdict != AC) entry.problems[att.problem-'A'].first = -1;
      else {
        entry.problems[att.problem-'A'].first = 1;
        entry.problems[att.problem-'A'].second = att.when;
        entry.ACs.push_back(att.when);
      }
    }
    else {
      problem_t& p = it->second.problems[att.problem-'A'];
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
  sort(entries.begin(), entries.end());
  
  // update back buffer
  (*backbuf) = "<tr><th>#</th><th>Team</th>";
  for (int i = 0; i < nproblems; i++) {
    (*backbuf) += "<th>";
    (*backbuf) += char(i+'A');
    (*backbuf) += "</th>";
  }
  (*backbuf) += "<th>Score</th></tr>";
  int place = 1;
  for (Entry& x : entries) {
    (*backbuf) += ("<tr><td>"+to<string>(place++)+"</td>");
    (*backbuf) += x.str();
    (*backbuf) += "</tr>";
  }
  
  // swap buffers
  string* tmp = backbuf;
  backbuf = frontbuf;
  pthread_mutex_lock(&frontbuf_mutex);
  frontbuf = tmp;
  pthread_mutex_unlock(&frontbuf_mutex);
}

namespace Scoreboard {

void update() {
  static time_t nextupd = 0;
  if (time(nullptr) < nextupd) return;
  ::update();
  nextupd = time(nullptr)+5;
}

string query(bool freeze) {
  // make local copy of scoreboard
  pthread_mutex_lock(&frontbuf_mutex);
  string scoreboard(*frontbuf);
  pthread_mutex_unlock(&frontbuf_mutex);
  
  string frozen;
  if (freeze) frozen = " (frozen)";
  return
    "<h2>Scoreboard"+frozen+"</h2>\n"
    "<table class=\"data\">"+
    scoreboard+
    "</table>"
  ;
}

} // namespace Scoreboard
