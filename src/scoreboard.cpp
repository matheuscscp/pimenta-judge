#include <cstdio>
#include <cstring>
#include <list>
#include <map>
#include <algorithm>

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "scoreboard.h"

#include "global.h"

using namespace std;

static string buf1, buf2;
static string* frontbuf = &buf1;
static string* backbuf = &buf2;
static pthread_mutex_t frontbuf_mutex = PTHREAD_MUTEX_INITIALIZER;

static void* client(void* ptr) {
  // fetch socket
  int* sdptr = (int*)ptr;
  int sd = *sdptr;
  delete sdptr;
  
  // ignore data
  char* buf = new char[1 << 10];
  while (read(sd, buf, 1 << 10) == 1 << 10);
  delete[] buf;
  
  // make local copy of scoreboard
  pthread_mutex_lock(&frontbuf_mutex);
  string scoreboard(*frontbuf);
  pthread_mutex_unlock(&frontbuf_mutex);
  
  // respond
  string response =
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\r"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<html><head><meta http-equiv=\"refresh\" content=\"10\" /></head>"
    "<body><table align=\"center\" border=\"3\">"+
    scoreboard+
    "<h1 align=\"center\">Pimenta Judgezzz~*~*</h1>"
    "<h2 align=\"center\">Scoreboard</h2>"
    "</table></body></html>"
  ;
  write(sd, response.c_str(), response.size());
  
  // close
  close(sd);
  return nullptr;
}

static void* server(void*) {
  // create socket
  int sd = socket(AF_INET, SOCK_STREAM, 0);
  int opt = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
  fcntl(sd, F_SETFL, FNDELAY);
  
  // set addr
  sockaddr_in addr;
  memset(&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(to<uint16_t>(Global::arg[3]));
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(sd, (sockaddr*)&addr, sizeof addr);
  
  // listen
  listen(sd, SOMAXCONN);
  while (Global::alive()) {
    int csd = accept(sd, nullptr, nullptr);
    if (csd < 0) { usleep(25000); continue; }
    pthread_t thread;
    pthread_create(&thread, nullptr, client, new int(csd));
    pthread_detach(thread);
  }
  
  // close
  close(sd);
  return nullptr;
}

static void update() {
  typedef pair<int, time_t> problem_t;
  struct Entry {
    string team;
    vector<problem_t> problems;
    int solved;
    int penalty;
    void compute_score(time_t begin) {
      solved = 0;
      penalty = 0;
      for (const problem_t& p : problems) {
        if (p.first <= 0) continue;
        solved++;
        penalty += (int(round((p.second - begin)/60.0)) + 20*(p.first-1));
      }
    }
    bool operator<(const Entry& other) const {
      return
        solved > other.solved ||
        (solved == other.solved && penalty < other.penalty)
      ;
    }
    string str() const {
      string ret = "<tr>";
      ret += ("<td>"+team+"</td>");
      for (const problem_t& p : problems) {
        ret += "<td align=\"center\" width=\"70px\">";
        if (p.first == 0)     ret += '-';
        else if (p.first > 0) ret += "OK (" + to<string>( p.first) + ")";
        else                  ret +=    "(" + to<string>(-p.first) + ")";
        ret += "</td>";
      }
      ret += ("<td>"+to<string>(solved)+" ("+to<string>(penalty)+")"+"</td>");
      return ret+"</tr>";
    }
  };
  
  // read file
  Settings settings;
  list<Attempt> atts; Attempt att;
  Global::lock_att_file();
  FILE* fp = fopen("attempts.bin", "rb");
  if (fp) {
    while (fread(&att, sizeof att, 1, fp) == 1) {
      if (att.when < settings.freeze) atts.push_back(att);
    }
    fclose(fp);
  }
  Global::unlock_att_file();
  
  // compute entries
  map<string, Entry> entriesmap;
  for (Attempt& att : atts) {
    auto it = entriesmap.find(att.team);
    if (it == entriesmap.end()) {
      Entry& entry = entriesmap[att.team];
      entry.team = att.team;
      entry.problems = vector<problem_t>(
        settings.problems.size(), make_pair(0, 0)
      );
      if (att.verdict != AC) entry.problems[att.problem].first = -1;
      else {
        entry.problems[att.problem].first = 1;
        entry.problems[att.problem].second = att.when;
      }
    }
    else {
      problem_t& p = it->second.problems[att.problem];
      if (p.first > 0) continue;
      if (att.verdict != AC) p.first--;
      else {
        p.first  = 1 - p.first;
        p.second = att.when;
      }
    }
  }
  
  // sort entries
  vector<Entry> entries;
  for (auto& kv : entriesmap) {
    kv.second.compute_score(settings.begin);
    entries.push_back(kv.second);
  }
  sort(entries.begin(), entries.end());
  
  // update back buffer
  (*backbuf) = "<tr><th>Team</th>";
  for (int i = 0; i < int(settings.problems.size()); i++) {
    (*backbuf) += "<th>";
    (*backbuf) += char(i+'A');
    (*backbuf) += "</th>";
  }
  (*backbuf) += "<th>Score</th></tr>";
  for (Entry& x : entries) (*backbuf) += x.str();
  
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
  Global::fire(server);
  Global::fire(poller);
}

} // namespace Scoreboard
