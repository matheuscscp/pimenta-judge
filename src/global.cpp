#include <unistd.h>
#include <signal.h>

#include "global.hpp"

#include "helper.hpp"
#include "database.hpp"
#include "message.hpp"
#include "judge.hpp"
#include "webserver.hpp"

using namespace std;

static bool quit = false;

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
    }
    static key_t alive() {
      FILE* fp = fopen("contest.bin", "rb");
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
    MessageQueue msq;
};

static void term(int) {
  Global::shutdown();
}

static key_t online() {
  key_t key = Contest::alive();
  if (!key) {
    printf("pjudge is not running: online operations can't be done.\n");
    _exit(0);
  }
  return key;
}

static void offline() {
  if (Contest::alive()) {
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
  printf("pjudge[%s] started.\n",getcwd().c_str());
  if (daemon(1,0) < 0) { // fork and redirect IO to /dev/null
    perror(stringf(
      "pjudge[%s] could not start in background",
      getcwd().c_str()
    ).c_str());
    _exit(-1);
  }
  Contest contest; // RAII
  signal(SIGTERM,term); // Global::shutdown();
  signal(SIGPIPE,SIG_IGN); // ignore broken pipes (tcp shit)
  Database::init();
  Judge::init();
  WebServer::init();
  while (!quit) {
    contest.update();
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
