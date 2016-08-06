#include <cstring>
#include <fstream>
#include <list>

#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/stat.h>

#include "global.hpp"

#include "message.hpp"
#include "judge.hpp"
#include "runlist.hpp"
#include "scoreboard.hpp"
#include "webserver.hpp"

using namespace std;

int verdict_toi(const string& verd) {
  if (verd ==  "AC") return AC;
  if (verd ==  "CE") return CE;
  if (verd == "RTE") return RTE;
  if (verd == "TLE") return TLE;
  if (verd ==  "WA") return WA;
  if (verd ==  "PE") return PE;
  return -1;
}

string verdict_tos(int verd) {
  switch (verd) {
    case  AC: return "AC";
    case  CE: return "CE";
    case RTE: return "RTE";
    case TLE: return "TLE";
    case  WA: return "WA";
    case  PE: return "PE";
  }
  return "";
}

string verdict_tolongs(int verd) {
  switch (verd) {
    case  AC: return "Accepted";
    case  CE: return "Compile Error";
    case RTE: return "Runtime Error";
    case TLE: return "Time Limit Exceeded";
    case  WA: return "Wrong Answer";
    case  PE: return "Presentation Error";
  }
  return "";
}

string balloon_img(char p) {
  string ret = "<img src=\"balloon.svg\" class=\"svg balloon ";
  ret += p;
  return ret + "\" />";
}

bool Attempt::read(FILE* fp) {
  vector<string> fields;
  for (int i = 0; i < 8 && !feof(fp); i++) {
    fields.emplace_back();
    string& s = fields.back();
    for (char c = fgetc(fp); c != ',' && !feof(fp); c = fgetc(fp)) s += c;
  }
  status = "";
  for (char c = fgetc(fp); c != '\n' && !feof(fp); c = fgetc(fp)) status += c;
  if (feof(fp)) return false;
  int f = 0;
  id = to<int>(fields[f++]);
  problem = fields[f++][0];
  verdict = verdict_toi(fields[f++]);
  when = to<int>(fields[f++]);
  runtime = fields[f++];
  username = fields[f++];
  ip = fields[f++];
  teamname = fields[f++];
  return true;
}

void Attempt::write(FILE* fp) const {
  fprintf(fp,"%d,",id);
  fprintf(fp,"%c,",problem);
  fprintf(fp,"%s,",verdict_tos(verdict).c_str());
  fprintf(fp,"%d,",when);
  fprintf(fp,"%s,",runtime.c_str());
  fprintf(fp,"%s,",username.c_str());
  fprintf(fp,"%s,",ip.c_str());
  fprintf(fp,"%s,",teamname.c_str());
  fprintf(fp,"%s\n",status.c_str());
}

bool Attempt::operator<(const Attempt& other) const {
  if (when != other.when) return when < other.when;
  return id < other.id;
}

string Attempt::toHTMLtr(bool blind, bool is_first) const {
  string ans = "<tr>";
  ans += "<td>"+to<string>(id)+"</td>";
  ans += "<td>"+to<string>(problem)+"</td>";
  ans += "<td>"+to<string>(when)+"</td>";
  if (blind) ans += "<td>Blind attempt</td>";
  else {
    ans += "<td>";
    if (is_first) ans += balloon_img(problem);
    else if (status != "judged") ans += "Not answered yet";
    else ans += verdict_tolongs(verdict);
    ans += "</td>";
  }
  return ans+"</tr>";
}

string Attempt::getHTMLtrheader() {
  return
    "<tr>"
      "<th>ID</th>"
      "<th>Problem</th>"
      "<th>Time (m)</th>"
      "<th>Answer</th>"
    "</tr>"
  ;
}

static bool quit = false;
static JSON settings;
static pthread_mutex_t settings_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t attfile_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t nextidfile_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t questionfile_mutex = PTHREAD_MUTEX_INITIALIZER;

class Contest {
  public:
    Contest() {
      key_t key = msq.key();
      FILE* fp = fopen("contest.bin", "wb");
      fwrite(&key,sizeof key,1,fp);
      fclose(fp);
    }
    ~Contest() {
      remove("contest.bin");
    }
    void update() {
      Runlist::update();
      Scoreboard::update();
      msq.update();
    }
    static key_t alive() {
      FILE* fp = fopen("contest.bin", "rb");
      if (!fp) return 0;
      key_t key;
      fread(&key,sizeof key,1,fp);
      fclose(fp);
      MessageQueue msq;
      Message(PING,msq.key()).send(key);
      if (msq.receive(3).mtype != IMALIVE) return 0;
      return key;
    }
  private:
    MessageQueue msq;
};

static void term(int) {
  Global::shutdown();
}

void lock_settings() {
  pthread_mutex_lock(&settings_mutex);
}

void unlock_settings() {
  pthread_mutex_unlock(&settings_mutex);
}

JSON& settings_ref() {
  return settings;
}

namespace Global {

void install(const string& dir) {
  system("cp -rf /usr/local/share/pjudge %s", dir.c_str());
}

void start() {
  if (Contest::alive()) {
    printf("pjudge is already running.\n");
    return;
  }
  printf("pjudge started.\n");
  if (daemon(1,0) < 0) { // fork with IO redirection to /dev/null
    perror("pjudge stopped because daemon() failed");
    _exit(-1);
  }
  Contest contest; // RAII
  signal(SIGTERM, term); // Global::shutdown();
  signal(SIGPIPE, SIG_IGN); // avoid broken pipes termination signal
  load_settings();
  pthread_t judge, webserver;
  pthread_create(&judge,nullptr,Judge::thread,nullptr);
  pthread_create(&webserver,nullptr,WebServer::thread,nullptr);
  while (!quit) {
    contest.update();
    usleep(25000);
  }
  pthread_join(judge,nullptr);
  pthread_join(webserver,nullptr);
}

void stop() {
  key_t key = Contest::alive();
  if (!key) {
    printf("pjudge is not running.\n");
    return;
  }
  Message(STOP).send(key);
  MessageQueue msq;
  Message ping(PING,msq.key());
  do {
    ping.send(key);
  } while (msq.receive(1).mtype == IMALIVE);
  printf("pjudge stopped.\n");
}

void reload_settings() {
  key_t key = Contest::alive();
  if (!key) {
    printf("pjudge is not running.\n");
    return;
  }
  Message(RELOAD).send(key);
  printf("pjudge reloaded settings.\n");
}

void lock_att_file() {
  pthread_mutex_lock(&attfile_mutex);
}

void unlock_att_file() {
  pthread_mutex_unlock(&attfile_mutex);
}

void lock_nextid_file() {
  pthread_mutex_lock(&nextidfile_mutex);
}

void unlock_nextid_file() {
  pthread_mutex_unlock(&nextidfile_mutex);
}

void lock_question_file() {
  pthread_mutex_lock(&questionfile_mutex);
}

void unlock_question_file() {
  pthread_mutex_unlock(&questionfile_mutex);
}

bool alive() {
  return !quit;
}

void shutdown() {
  quit = true;
}

void load_settings() {
  pthread_mutex_lock(&settings_mutex);
  ::settings.read_file("settings.json");
  JSON& contest = ::settings("contest");
  // begin
  auto& start = contest("start");
  int Y = start("year");
  int M = start("month");
  int D = start("day");
  int h = start("hour");
  int m = start("minute");
  contest.erase("start");
  time_t begin = time(nullptr);
  tm ti;
  localtime_r(&begin,&ti);
  ti.tm_year = Y - 1900;
  ti.tm_mon  = M - 1;
  ti.tm_mday = D;
  ti.tm_hour = h;
  ti.tm_min  = m;
  ti.tm_sec  = 0;
  begin = mktime(&ti);
  contest("begin") = begin;
  // end
  time_t end = contest("duration");
  contest.erase("duration");
  end = begin + 60*end;
  contest("end") = end;
  // freeze
  time_t freeze = contest("freeze");
  freeze = end - 60*freeze;
  contest("freeze") = freeze;
  // blind
  time_t blind = contest("blind");
  blind = end - 60*blind;
  contest("blind") = blind;
  // languages
  JSON& langs = contest("languages");
  for (auto it = langs.begin_o(); it != langs.end_o();) {
    if (!it->second("enabled")) langs.erase(it++);
    else {
      it->second.erase("enabled");
      it++;
    }
  }
  // problems
  JSON& probs = contest("problems");
  for (int i = 0; i < probs.size();) {
    if (!probs[i]("enabled")) probs.erase(probs.begin_a()+i);
    else {
      probs[i].erase("enabled");
      i++;
    }
  }
  pthread_mutex_unlock(&settings_mutex);
}

time_t remaining_time() {
  JSON contest(move(Global::settings("contest")));
  time_t begin = contest("begin");
  time_t end = contest("end");
  time_t now = time(nullptr);
  return (now < begin ? 0 : max(0,int(end-now)));
}

} // namespace Global
