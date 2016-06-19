#include <cstring>
#include <fstream>
#include <list>

#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/stat.h>

#include "global.h"

#include "judge.h"
#include "scoreboard.h"
#include "rejudger.h"
#include "webserver.h"

using namespace std;

static pthread_mutex_t localtime_mutex = PTHREAD_MUTEX_INITIALIZER;
Settings::Settings() {
  memset(this, 0, sizeof(::Settings));
  fstream f("settings.txt");
  if (!f.is_open()) return;
  string tmp;
  auto func = [&](time_t& buf) {
    int Y, M, D, h, m;
    f >> tmp >> Y >> M >> D >> h >> m;
    time(&buf);
    pthread_mutex_lock(&localtime_mutex);
    tm* tinfo = localtime(&buf);
    tm ti = *tinfo;
    pthread_mutex_unlock(&localtime_mutex);
    ti.tm_year = Y - 1900;
    ti.tm_mon  = M - 1;
    ti.tm_mday = D;
    ti.tm_hour = h;
    ti.tm_min  = m;
    ti.tm_sec  = 0;
    buf = mktime(&ti);
  };
  func(begin);
  func(end);
  func(freeze);
  func(noverdict);
  int timelimit;
  while (f >> tmp >> timelimit) problems.push_back(timelimit);
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
  pthread_mutex_lock(&attfile_mutex);
  pthread_mutex_lock(&nextidfile_mutex);
  pthread_mutex_lock(&questionfile_mutex);
  quit = true;
  pthread_mutex_unlock(&questionfile_mutex);
  pthread_mutex_unlock(&nextidfile_mutex);
  pthread_mutex_unlock(&attfile_mutex);
  for (pthread_t& thread : threads) pthread_join(thread, nullptr);
  exit(0);
}

namespace Global {

vector<string> arg;

void install(const string& dir) {
  mkdir(dir.c_str(), 0777);
  FILE* fp = fopen((dir+"/settings.txt").c_str(), "w");
  fprintf(fp, 
    "Begin:  2015 09 01 19 00\n"
    "End:    2015 12 25 23 50\n"
    "Freeze: 2015 12 25 23 50\n"
    "Blind:  2015 12 25 23 50\n"
    "A(time-limit-per-file-in-seconds): 4\n"
    "B(the-alphabetical-order-must-be-followed): 3\n"
    "C(these-comments-are-useless-and-can-be-removed): 5\n"
    "D: 1\n"
  );
  fclose(fp);
  fp = fopen((dir+"/teams.txt").c_str(), "w");
  fprintf(fp,
    "\"Team 1 Name\" team1username team1password\n"
    "\"Team 2 Name\" team1username team2password\n"
    "\"Team 3 Name\" team1username team3password\n"
  );
  fclose(fp);
  fp = fopen((dir+"/clarifications.txt").c_str(), "w");
  fprintf(fp,
    "global A \"Problem A question available to all teams\" \"Answer\"\n"
    "team1username C \"Problem C question privately answered to team1username\""
    " \"Answer\"\n"
  );
  fclose(fp);
  mkdir((dir+"/problems").c_str(), 0777);
  fp = fopen((dir+"/problems/A.in1").c_str(), "w");
  fclose(fp);
  fp = fopen((dir+"/problems/A.sol1").c_str(), "w");
  fclose(fp);
  fp = fopen((dir+"/problems/B.in1").c_str(), "w");
  fclose(fp);
  fp = fopen((dir+"/problems/B.sol1").c_str(), "w");
  fclose(fp);
  fp = fopen((dir+"/problems/C.in1").c_str(), "w");
  fclose(fp);
  fp = fopen((dir+"/problems/C.sol1").c_str(), "w");
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
  Scoreboard::fire();
  Rejudger::fire(msqid);
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
