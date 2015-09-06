#ifndef GLOBAL_H
#define GLOBAL_H

#include <vector>
#include <string>
#include <sstream>

enum {AC = 0, CE, RTE, TLE, WA, PE};

struct Settings {
  time_t begin, end, freeze, noverdict;
  std::vector<int> problems;
  Settings();
};

struct Attempt {
  int id;
  char team[128];
  char problem;
  char verdict;
  time_t when;
};

template <typename NewType, typename T>
NewType to(const T& x) {
  std::stringstream ss; ss << x;
  NewType ret; ss >> ret;
  return ret;
}

template <typename... Args>
int system(const char* fmt, Args... args) {
  char cmd[256];
  sprintf(cmd, fmt, args...);
  return ::system(cmd);
}

int timeout(bool&, int, const char*);
template <typename... Args>
int timeout(bool& tle, int s, const char* fmt, Args... args) {
  char cmd[256];
  sprintf(cmd, fmt, args...);
  return timeout(tle, s, cmd);
}

namespace Global {

extern std::vector<std::string> arg;

void install(const std::string&);
void start(int, char**);
void stop();
void rejudge(int, char);

bool alive();
void fire(void* (*)(void*));

void lock_att_file();
void unlock_att_file();
void lock_nextid_file();
void unlock_nextid_file();

} // namespace Global

#endif
