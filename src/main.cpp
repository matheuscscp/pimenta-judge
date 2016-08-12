#include "global.hpp"

using namespace std;

static char* exe;
static void usagemode() {
  printf(
    "Usage mode: %s <option>\n"
    "\n"
    "Installation option:\n"
    "  install <non-existing directory path>\n"
    "\n"
    "The following options must be executed inside a directory created with\n"
    "the 'install' option.\n"
    "\n"
    "Offline options:\n"
    "  start\n"
    "\n"
    "Online options:\n"
    "  stop\n"
    "  restart (stop followed by start)\n"
    "  reload (reload all except webserver settings)\n"
    "  rerun-att <attempt id>\n"
    "  rerun-probs <list of problem id ranges> (* reruns ALL attempts)\n"
    "    Example to rerun problems 1, 3, 4, 5 and 6:\n"
    "    %s rerun-probs 1 3-6\n",
    exe,exe
  );
}

int main(int argc, char** argv) {
  exe = argv[0];
  if (argc <= 1) { usagemode(); return 0; }
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
  else if (mode == "restart") {
    Global::stop();
    Global::start();
  }
  else if (mode == "reload") {
    Global::reload();
  }
  else if (mode == "rerun-att") {
    if (argc == 2) { usagemode(); return 0; }
    int id;
    if (sscanf(argv[2],"%d",&id) != 1) { usagemode(); return 0; }
    Global::rerun_attempt(id);
  }
  else usagemode();
  return 0;
}
