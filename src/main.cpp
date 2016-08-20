#include <map>
#include <string>
#include <functional>
#include <vector>

#include "global.hpp"

using namespace std;

static char* exe;
static void usage() {
  printf(
    "Usage: %s <option>\n"
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
    "  restart (precisely a stop followed by a start)\n"
    "  rerun-att <attempt id>\n"
    "  rerun-probs <list of problem id ranges> (* reruns ALL attempts)\n"
    "    Example to rerun problems 1, 3, 4, 5 and 6:\n"
    "    %s rerun-probs 1 3-6\n",
    exe,exe
  );
  exit(0);
}

int main(int argc, char** argv) {
  map<string,map<int,function<void(const vector<string>&)>>> funcs;
  funcs["install"][1] = [](const vector<string>& args) {
    Global::install(args[0]);
  };
  funcs["start"][0] = [](const vector<string>&) {
    Global::start();
  };
  funcs["stop"][0] = [](const vector<string>&) {
    Global::stop();
  };
  funcs["restart"][0] = [](const vector<string>&) {
    Global::stop();
    Global::start();
  };
  funcs["rerun-att"][1] = [](const vector<string>& args) {
    int id;
    if (sscanf(args[0].c_str(),"%d",&id) != 1) usage();
    Global::rerun_attempt(id);
  };
  exe = argv[0];
  if (argc <= 1) usage();
  auto functor = funcs.find(argv[1]);
  if (functor == funcs.end()) usage();
  auto arity = functor->second.find(argc-2);
  if (arity == functor->second.end()) usage();
  vector<string> args;
  for (int i = 2; i < argc; i++) args.push_back(argv[i]);
  arity->second(args);
  return 0;
}
