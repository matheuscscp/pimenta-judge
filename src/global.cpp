#include <cstring>
#include <fstream>
#include <list>

#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/stat.h>

#include "global.hpp"

#include "judge.hpp"
#include "runlist.hpp"
#include "scoreboard.hpp"
#include "rejudger.hpp"
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

struct Contest {
  pid_t pid;
  key_t key;
  Contest(pid_t pid, key_t key) : pid(pid), key(key) {
    FILE* fp = fopen("contest.bin", "wb");
    fwrite(this, sizeof(Contest), 1, fp);
    fclose(fp);
  }
  Contest() : pid(0), key(0) {
    FILE* fp = fopen("contest.bin", "rb");
    if (!fp) return;
    fread(this, sizeof(Contest), 1, fp);
    fclose(fp);
  }
};

static bool quit = false;
static list<pthread_t> threads;
static pthread_mutex_t attfile_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t nextidfile_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t questionfile_mutex = PTHREAD_MUTEX_INITIALIZER;
static int msqid;

static key_t msgqueue() {
  key_t key = 0x713e24a;
  while ((msqid = msgget(key, (IPC_CREAT|IPC_EXCL)|0777)) < 0) key++;
  return key;
}

static void term(int) {
  msgctl(msqid, IPC_RMID, nullptr);
  remove("contest.bin");
  quit = true;
  for (pthread_t& thread : threads) pthread_join(thread, nullptr);
  exit(0);
}

namespace Global {

vector<string> arg;

void install(const string& dir) {
  mkdir(dir.c_str(), 0777);
  FILE* fp = fopen((dir+"/settings.txt").c_str(), "w");
  fprintf(fp, 
    "Start:    2015 09 01 19 00\n"
    "Duration: 300\n"
    "Freeze:   60\n"
    "Blind:    15\n"
    "C:        enabled\n"
    "C++:      enabled\n"
    "Java:     enabled\n"
    "Python:   disabled\n"
    "Python3:  disabled\n"
    "A:        4 autojudge\n"
    "B:        3 manual\n"
  );
  fclose(fp);
  fp = fopen((dir+"/teams.txt").c_str(), "w");
  fprintf(fp,
    "\"Team 1\" team1username team1password\n"
    "\"Team 2\" team2username team2password\n"
    "\"Team 3\" team3username team3password\n"
  );
  fclose(fp);
  fp = fopen((dir+"/clarifications.txt").c_str(), "w");
  fprintf(fp,
    "global A \"Problem A question available to all teams\" \"Answer\"\n"
    "team1username B \"Problem B question privately answered to Team 1\""
    " \"Answer\"\n"
  );
  fclose(fp);
  mkdir((dir+"/problems").c_str(), 0777);
  mkdir((dir+"/problems/A").c_str(), 0777);
  mkdir((dir+"/problems/A/input").c_str(), 0777);
  mkdir((dir+"/problems/A/output").c_str(), 0777);
  mkdir((dir+"/problems/B").c_str(), 0777);
  mkdir((dir+"/problems/B/input").c_str(), 0777);
  mkdir((dir+"/problems/B/output").c_str(), 0777);
  fp = fopen((dir+"/problems/A/input/file1_huge_case").c_str(), "w");
  fclose(fp);
  fp = fopen((dir+"/problems/A/input/file2").c_str(), "w");
  fclose(fp);
  fp = fopen((dir+"/problems/A/output/file1_huge_case").c_str(), "w");
  fclose(fp);
  fp = fopen((dir+"/problems/A/output/file2").c_str(), "w");
  fclose(fp);
  fp = fopen((dir+"/problems/B/input/file1").c_str(), "w");
  fclose(fp);
  fp = fopen((dir+"/problems/B/output/file1").c_str(), "w");
  fclose(fp);
  system("cp -rf /usr/local/share/pjudge/www %s/www", dir.c_str());
}

void start(int argc, char** argv) {
  if (Contest().pid) return;
  arg = vector<string>(argc);
  for (int i = 0; i < argc; i++) arg[i] = argv[i];
  daemon(1,0);
  sleep(1); // if child runs before parent... shit happens
  Contest(getpid(), msgqueue());
  signal(SIGTERM, term);
  signal(SIGPIPE, SIG_IGN);
  Judge::fire();
  Runlist::fire();
  Scoreboard::fire();
  WebServer::fire();
  for (pthread_t& thread : threads) pthread_join(thread, nullptr);
}

void stop() {
  Contest c;
  if (c.pid) kill(c.pid, SIGTERM);
}

void rejudge(int id, char verdict) {
  Contest c;
  if (!c.pid) Rejudger::rejudge(id, verdict);
  else {
    rejudgemsg msg(id, verdict);
    msgsnd(msgget(c.key, 0), &msg, msg.size(), 0);
  }
}

bool alive() {
  return !quit;
}

void fire(void* (*thread)(void*)) {
  threads.emplace_back();
  pthread_create(&threads.back(), nullptr, thread, nullptr);
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

} // namespace Global
