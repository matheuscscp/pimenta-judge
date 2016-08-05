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

Settings::Settings() {
  memset(this, 0, sizeof(::Settings));
  fstream f("settings.txt");
  if (!f.is_open()) return;
  string tmp,tmp2;
  {
    int Y, M, D, h, m;
    f >> tmp >> Y >> M >> D >> h >> m;
    begin = time(nullptr);
    tm ti;
    localtime_r(&begin,&ti);
    ti.tm_year = Y - 1900;
    ti.tm_mon  = M - 1;
    ti.tm_mday = D;
    ti.tm_hour = h;
    ti.tm_min  = m;
    ti.tm_sec  = 0;
    begin = mktime(&ti);
  }
  f >> tmp >> end;
  end = begin + 60*end;
  f >> tmp >> freeze;
  freeze = end - 60*freeze;
  f >> tmp >> blind;
  blind = end - 60*blind;
  set<string> langs;
  f >> tmp >> tmp2;
  if (tmp2 == "enabled") langs.insert(".c");
  f >> tmp >> tmp2;
  if (tmp2 == "enabled") langs.insert(".cpp");
  f >> tmp >> tmp2;
  if (tmp2 == "enabled") langs.insert(".java");
  f >> tmp >> tmp2;
  if (tmp2 == "enabled") langs.insert(".py");
  f >> tmp >> tmp2;
  if (tmp2 == "enabled") langs.insert(".py3");
  this->langs = langs;
  for (Problem p; f >> tmp >> p.timelimit >> tmp2;) {
    p.autojudge = (tmp2 == "autojudge");
    problems.push_back(p);
  }
}

string Settings::enabled_langs() const {
  string ans =
    "<table class=\"data\">"
      "<tr><th>Language</th><th>File extension</th><th>Flags</th></tr>"
  ;
  if (langs.find(".c") != langs.end())
    ans += "<tr><td>C</td><td>.c</td><td>-std=c11 -lm</td></tr>";
  if (langs.find(".cpp") != langs.end())
    ans += "<tr><td>C++</td><td>.cpp</td><td>-std=c++1y</td></tr>";
  if (langs.find(".java") != langs.end())
    ans += "<tr><td>Java</td><td>.java</td><td></td></tr>";
  if (langs.find(".py") != langs.end())
    ans += "<tr><td>Python</td><td>.py</td><td></td></tr>";
  if (langs.find(".py3") != langs.end())
    ans += "<tr><td>Python 3</td><td>.py3</td><td></td></tr>";
  return ans+"</table>";
}

string Settings::limits() const {
  string ans =
    "<table class=\"data\">"
      "<tr><th>Problem</th>"
  ;
  for (int i = 0; i < problems.size(); i++) {
    ans += "<th>"+to<string>(char(i+'A'))+"</th>";
  }
  ans +=
      "</tr>"
      "<tr><th>Time limit (s)</th>"
  ;
  for (int i = 0; i < problems.size(); i++) {
    ans += "<td>"+to<string>(problems[i].timelimit)+"</td>";
  }
  ans +=
      "</tr>"
    "</table>"
  ;
  return ans;
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
  ans += "<td>"+(verdict == TLE ? verdict_tolongs(TLE) : runtime)+"</td>";
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
      "<th>Execution time (s)</th>"
      "<th>Answer</th>"
    "</tr>"
  ;
}

rejudgemsg::rejudgemsg(int id, char verdict)
: mtype(1), id(id), verdict(verdict) {}

size_t rejudgemsg::size() const { return sizeof(rejudgemsg)-sizeof(long); }

void ignoresd(int sd) {
  char* buf = new char[1 << 10];
  while (read(sd, buf, 1 << 10) == (1 << 10));
  delete[] buf;
}

template <>
string to<string, in_addr_t>(const in_addr_t& x) {
  char* ptr = (char*)&x;
  string ret = to<string>(int(ptr[0])&0x0ff);
  for (int i = 1; i < 4; i++) ret += ("."+to<string>(int(ptr[i])&0x0ff));
  return ret;
}

static bool quit = false;
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
    static bool alive() {
      FILE* fp = fopen("contest.bin", "rb");
      if (!fp) return false;
      key_t key;
      fread(&key,sizeof key,1,fp);
      fclose(fp);
      MessageQueue msq;
      Message(PING,msq.key()).send(key);
      return msq.receive(3).mtype == IMALIVE;
    }
    static bool stop() {
      FILE* fp = fopen("contest.bin", "rb");
      if (!fp) return false;
      key_t key;
      fread(&key,sizeof key,1,fp);
      fclose(fp);
      MessageQueue msq;
      Message ping(PING,msq.key());
      ping.send(key);
      if (msq.receive(3).mtype != IMALIVE) return false;
      Message(STOP).send(key);
      do {
        ping.send(key);
      } while (msq.receive(1).mtype == IMALIVE);
      return true;
    }
  private:
    MessageQueue msq;
};

static void term(int) {
  Global::shutdown();
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
  if (Contest::stop()) printf("pjudge stopped.\n");
  else printf("pjudge is not running.\n");
}

bool alive() {
  return !quit;
}

void shutdown() {
  quit = true;
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

time_t remaining_time() {
  Settings settings;
  time_t now = time(nullptr);
  return (now < settings.begin ? 0 : max(0,int(settings.end-now)));
}

} // namespace Global
