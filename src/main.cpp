#include "global.hpp"

using namespace std;

static char* exe;
static void usagemode() {
  printf(
    "Usage mode: %s [options...]\n"
    "\n"
    "Options:\n"
    "  install  <new directory name without trailing slash>\n"
    "  start\n"
    "  stop\n"
    "  gp       <list of problems by capital letter>\n"
    "  ga       <(possibly empty) list of problems by capital letter>\n"
    "  rid      <attempt id>\n"
    "  rp       <list of problems by capital letter>\n"
    "  ra       <(possibly empty) list of problems by capital letter>\n"
    "\n"
    "gp:  Generate expected outputs for a set of problems.\n"
    "ga   Generate expected outputs for ALL problems, except for a set.\n"
    "rid: Re-run attempt by id.\n"
    "rp:  Re-run ALL attempts for a set of problems.\n"
    "ra:  Re-run ALL attempts, except for a set of problems.\n",
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
  else usagemode();
  return 0;
}
