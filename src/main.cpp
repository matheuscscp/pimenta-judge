#include "global.hpp"

using namespace std;

static char* exe;
static void usagemode() {
  printf(
    "Usage mode: %s <option>\n"
    "\n"
    "Options:\n"
    "  install <new directory name without trailing slash>\n"
    "  start\n"
    "  stop\n"
    "  reload-settings (does not work for webserver settings)\n"
    "  gen-outs <list of problem names>\n"
    "  gen-all-outs [list of problem names to skip]\n"
    "  rerun-id <attempt id>\n"
    "  rerun-atts <list of problem names>\n"
    "  rerun-all-atts [list of problem names to skip]\n",
    exe
  );
}

int main(int argc, char** argv) {
  exe = argv[0];
  if (argc == 1) { usagemode(); return 0; }
  string mode = argv[1];
  if (mode == "install") {
    if (argc == 2) { usagemode(); return 0; }
    Global::install(argv[2]);
  }
  else if (mode == "start") {
    Global::start();
  }
  else if (mode == "stop") {
    Global::stop();
  }
  else if (mode == "reload-settings") {
    Global::reload_settings();
  }
  else usagemode();
  return 0;
}
