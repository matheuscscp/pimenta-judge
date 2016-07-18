#include <map>
#include <algorithm>

#include <unistd.h>

#include "scoreboard.h"

#include "global.h"

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
  Settings settings;
  vector<Attempt> atts; Attempt att;
  Global::lock_att_file();
  FILE* fp = fopen("attempts.txt", "r");
  if (fp) {
    while (att.read(fp)) {
      if (settings.begin + 60*att.when < settings.freeze) atts.push_back(att);
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
      entry.problems = vector<problem_t>(
        settings.problems.size(), make_pair(0, 0)
      );
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
  for (int i = 0; i < int(settings.problems.size()); i++) {
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

static void* poller(void*) {
  for (time_t nextupd = 0; Global::alive();) {
    if (time(nullptr) < nextupd) { usleep(25000); continue; }
    update();
    nextupd = time(nullptr) + 5;
  }
  return nullptr;
}

namespace Scoreboard {

void fire() {
  Global::fire(poller);
}

void send(int sd, bool freeze) {
  // make local copy of scoreboard
  pthread_mutex_lock(&frontbuf_mutex);
  string scoreboard(*frontbuf);
  pthread_mutex_unlock(&frontbuf_mutex);
  
  // respond
  string frozen;
  if (freeze) frozen = " (frozen)";
  string response =
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\r"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<h2>Scoreboard"+frozen+"</h2>\n"
    "<table class=\"data\">"+
    scoreboard+
    "</table>"
  ;
  write(sd, response.c_str(), response.size());
}

} // namespace Scoreboard
