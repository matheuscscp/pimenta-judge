#include <map>
#include <algorithm>

#include <unistd.h>

#include "runlist.hpp"

#include "global.hpp"

using namespace std;

static map<string, string> buf1, buf2;
static map<string, string>* frontbuf = &buf1;
static map<string, string>* backbuf = &buf2;
static pthread_mutex_t frontbuf_mutex = PTHREAD_MUTEX_INITIALIZER;

static void update() {
  // read file
  Settings settings;
  vector<Attempt> atts;
  Global::lock_att_file();
  FILE* fp = fopen("attempts.txt", "r");
  if (fp) {
    Attempt att;
    while (att.read(fp)) atts.push_back(att);
    fclose(fp);
  }
  Global::unlock_att_file();
  stable_sort(atts.begin(),atts.end());
  
  // update back buffer
  map<string, string>& buf = *backbuf;
  map<string, set<char>> ACs;
  buf.clear();
  for (auto& att : atts) {
    string& s = buf[att.username];
    if (s == "") s = Attempt::getHTMLtrheader();
    auto& S = ACs[att.username];
    bool blind = settings.blind <= settings.begin + 60*att.when;
    bool is_first =
      S.find(att.problem) == S.end() &&
      att.verdict == AC &&
      att.status == "judged"
    ;
    s += att.toHTMLtr(blind,is_first);
    if (is_first) S.insert(att.problem);
  }
  
  // swap buffers
  map<string, string>* tmp = backbuf;
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

namespace Runlist {

void fire() {
  Global::fire(poller);
}

string query(const string& username) {
  // make local copy of runlist
  pthread_mutex_lock(&frontbuf_mutex);
  string runlist((*frontbuf)[username]);
  pthread_mutex_unlock(&frontbuf_mutex);
  
  return
    "<h2>Attempts</h2>\n"
    "<table id=\"attempts-table\" class=\"data\">"+
    runlist+
    "</table>"
  ;
}

} // namespace Runlist
