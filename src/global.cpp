#include <cstring>
#include <fstream>
#include <list>

#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/stat.h>

#include "global.hpp"

#include "helper.hpp"
#include "message.hpp"
#include "judge.hpp"
#include "runlist.hpp"
#include "scoreboard.hpp"
#include "webserver.hpp"

using namespace std;

static bool quit = false;
static JSON settings;
static pthread_mutex_t settings_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t attempts_mutex = PTHREAD_MUTEX_INITIALIZER;
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
      msq.update();
      static time_t nxtupd = 0;
      if (time(nullptr) < nxtupd) return;
      pthread_mutex_lock(&attempts_mutex);
      JSON atts(Global::attempts);
      pthread_mutex_unlock(&attempts_mutex);
      atts.write_file("attempts.json");
      Runlist::update(atts);
      Scoreboard::update(atts);
      nxtupd = time(nullptr)+5;
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

JSON attempts;

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
  if (!attempts.read_file("attempts.json")) attempts = JSON();
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

void lock_attempts() {
  pthread_mutex_lock(&attempts_mutex);
}

void unlock_attempts() {
  pthread_mutex_unlock(&attempts_mutex);
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
  auto& langs = contest("languages").obj();
  for (auto it = langs.begin(); it != langs.end();) {
    if (!it->second("enabled")) langs.erase(it++);
    else {
      it->second.erase("enabled");
      it++;
    }
  }
  // problems
  JSON problems;
  for (auto& p : contest("problems").arr()) {
    if (!p("enabled") || problems.find_tuple(p("dirname"))) continue;
    int j = problems.size();
    problems(move(p("dirname").str())) = move(JSON(move(map<string,JSON>{
      {"index"    , j},
      {"timelimit", move(p("timelimit"))},
      {"autojudge", move(p("autojudge"))},
      {"color"    , move(p("color"))}
    })));
  }
  contest("problems") = move(problems);
  pthread_mutex_unlock(&settings_mutex);
}

} // namespace Global
