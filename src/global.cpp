#include <cstring>
#include <fstream>
#include <list>

#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/stat.h>

#include "global.h"

#include "judge.h"
#include "scoreboard.h"
#include "clarification.h"
#include "statement.h"
#include "rejudger.h"

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
  int problems;
  f >> tmp >> problems;
  this->problems = vector<int>(problems);
  for (int i = 0; i < problems; i++) {
    f >> tmp >> this->problems[i];
  }
}

rejudgemsg::rejudgemsg(int id, char verdict)
: mtype(1), id(id), verdict(verdict) {}

size_t rejudgemsg::size() const { return sizeof(rejudgemsg)-sizeof(long); }

int timeout(bool& tle, int s, const char* cmd) {
  tle = false;
  
  timeval start, end;
  gettimeofday(&start, nullptr);
  
  pid_t proc = fork();
  if (!proc) {
    setpgid(0, 0);
    exit(system(cmd));
  }
  
  int status;
  while (waitpid(proc, &status, WNOHANG) != proc) {
    gettimeofday(&end, nullptr);
    __suseconds_t dt =
      (end.tv_sec*1000000 + end.tv_usec)-
      (start.tv_sec*1000000 + start.tv_usec)
    ;
    if (dt >= s*1000000) {
      tle = true;
      kill(-proc, SIGKILL); //the minus kills the whole tree rooted at proc
      return 0;
    }
    usleep(10000);
  }
  
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  return -1;
}

void ignoresd(int sd) {
  char* buf = new char[1 << 10];
  while (read(sd, buf, 1 << 10) == (1 << 10));
  delete[] buf;
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
    "Begin:        2015 09 01 19 00\n"
    "End:          2015 12 25 23 50\n"
    "Freeze:       2015 12 25 23 50\n"
    "No-verdict:   2015 12 25 23 50\n"
    "Problems: 1\n"
    "A-timelimit: 1\n"
  );
  fclose(fp);
  fp = fopen((dir+"/teams.txt").c_str(), "w");
  fprintf(fp, "\"Team 1\" Team1 Team1Password\n");
  fclose(fp);
  fp = fopen((dir+"/clarifications.txt").c_str(), "w");
  fprintf(fp, "A \"The question must be between quotes\" \"The answer too\"\n");
  fclose(fp);
  mkdir((dir+"/problems").c_str(), 0777);
  fp = fopen((dir+"/problems/A.in").c_str(), "w");
  fclose(fp);
  fp = fopen((dir+"/problems/A.sol").c_str(), "w");
  fclose(fp);
}

void start(int argc, char** argv) {
  if (Contest().pid) return;
  arg = vector<string>(argc);
  for (int i = 0; i < argc; i++) arg[i] = argv[i];
  daemon(1, 0);
  Contest(getpid(), msgqueue());
  signal(SIGTERM, term);
  signal(SIGPIPE, SIG_IGN);
  Judge::fire();
  Scoreboard::fire();
  Clarification::fire();
  Statement::fire();
  Rejudger::fire(msqid);
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
