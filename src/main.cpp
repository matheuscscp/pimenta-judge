#include "global.h"

static void usagemode() {
  printf(
    "Usage mode:\n"
    "  install <new directory name without trailing slash>\n"
    "  start <judge port> <scoreboard port> <contest file port>\n"
    "  stop\n"
    "  rejudge <attempt id> <verdict number>\n"
    "Verdicts:\n"
    "  AC  = %d\n"
    "  CE  = %d\n"
    "  RTE = %d\n"
    "  TLE = %d\n"
    "  WA  = %d\n"
    "  PE  = %d\n",
    AC, CE, RTE, TLE, WA, PE
  );
}

int main(int argc, char** argv) {
  if (argc == 1) { usagemode(); return 0; }
  std::string mode = argv[1];
  if (mode == "install") {
    if (argc == 2) { usagemode(); return 0; }
    Global::install(argv[2]);
  }
  else if (mode == "start") {
    if (argc < 5) { usagemode(); return 0; }
    Global::start(argc, argv);
  }
  else if (mode == "stop") {
    Global::stop();
  }
  else if (mode == "rejudge") {
    if (argc < 4) { usagemode(); return 0; }
    Global::rejudge(to<int>(argv[2]), (char)to<int>(argv[3]));
  }
  else usagemode();
  return 0;
}
