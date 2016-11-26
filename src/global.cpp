#include <unistd.h>
#include <signal.h>

#include "global.hpp"

#include "helper.hpp"
#include "database.hpp"
#include "message.hpp"
#include "judge.hpp"
#include "webserver.hpp"
#include "contest.hpp"
#include "attempt.hpp"

using namespace std;

static bool quit = false;

class pjudge {
  public:
    pjudge() : sudden(sudden_shutdown()) {
      key_t key = msq.key();
      FILE* fp = fopen("pjudge.bin", "wb");
      fwrite(&key,sizeof key,1,fp);
      fclose(fp);
    }
    ~pjudge() {
      if (!sudden) remove("pjudge.bin");
    }
    void update() {
      msq.update();
    }
    static bool sudden_shutdown() {
      FILE* fp = fopen("pjudge.bin","rb");
      if (!fp) return false;
      fclose(fp);
      return true;
    }
    static key_t alive() {
      FILE* fp = fopen("pjudge.bin", "rb");
      if (!fp) return 0;
      key_t key;
      fread(&key,sizeof key,1,fp);
      fclose(fp);
      MessageQueue msq;
      PingMessage(msq.key()).send(key);
      if (msq.receive(3).mtype != IMALIVE) return 0;
      return key;
    }
  private:
    bool sudden;
    MessageQueue msq;
};

static void term(int) {
  Global::shutdown();
}

static key_t online() {
  key_t key = pjudge::alive();
  if (!key) {
    printf("pjudge is not running: online operations can't be done.\n");
    _exit(0);
  }
  return key;
}

static void offline() {
  if (pjudge::alive()) {
    printf("pjudge is running: offline operations can't be done.\n");
    _exit(0);
  }
}

namespace Global {

void install(const string& path) {
  FILE* fp = fopen(path.c_str(),"r");
  if (fp) {
    fclose(fp);
    printf("pjudge install failed: file already exists.\n");
    return;
  }
  system("mkdir -p %s",path.c_str());
  system("cp -rf /usr/local/share/pjudge/* %s",path.c_str());
  printf("pjudge installed at %s.\n",path.c_str());
}

void start() {
  offline();
  bool sudden = pjudge::sudden_shutdown();
  if (sudden) printf(
    "WARNING: last pjudge[%s] execution stopped suddenly.\n"
    "this execution will not make database backups.\n"
    "\n"
    "FIX: 1) check your data; 2) stop pjudge; 3) manually rollback the\n"
    "database if necessary; 4) remove file 'pjudge.bin'.\n"
    "\n"
    "next execution will be fine.\n",
    getcwd().c_str()
  );
  printf("pjudge[%s] started.\n",getcwd().c_str());
  if (daemon(1,0) < 0) { // fork and redirect IO to /dev/null
    perror(stringf(
      "pjudge[%s] could not start in background",
      getcwd().c_str()
    ).c_str());
    _exit(-1);
  }
  pjudge pj; // RAII
  signal(SIGTERM,term); // Global::shutdown();
  signal(SIGPIPE,SIG_IGN); // ignore broken pipes (tcp shit)
  Database::init(!sudden);
  Contest::fix();
  Attempt::fix();
  Judge::init();
  WebServer::init();
  while (!quit) {
    pj.update();
    usleep(25000);
  }
  WebServer::close();
  Judge::close();
  Database::close();
}

void stop() {
  key_t key = online();
  Message(STOP).send(key);
  MessageQueue msq;
  PingMessage ping(msq.key());
  do {
    ping.send(key);
  } while (msq.receive(1).mtype == IMALIVE);
  printf("pjudge[%s] stopped.\n",getcwd().c_str());
}

void rerun_attempt(int id) {
  RerunAttMessage(id).send(online());
  printf(
    "pjudge[%s] pushed attempt id=%d to queue.\n",
    getcwd().c_str(),
    id
  );
}

void shutdown() {
  quit = true;
}

} // namespace Global
