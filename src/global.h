#ifndef GLOBAL_H
#define GLOBAL_H

#include <vector>
#include <string>
#include <sstream>

#include <netinet/in.h>

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

struct rejudgemsg {
  long mtype;
  int id;
  char verdict;
  rejudgemsg(int, char);
  size_t size() const;
};

int timeout(bool&, int, const char*);
bool instance_exists(char problem, int i);
int timeout2(bool& tle, int s, const std::string& cmd, char problem, const std::string& outpref);
void ignoresd(int);

template <typename NewType, typename T>
NewType to(const T& x) {
  std::stringstream ss; ss << x;
  NewType ret; ss >> ret;
  return ret;
}
template <>
std::string to<std::string, in_addr_t>(const in_addr_t&);

template <typename... Args>
int system(const char* fmt, Args... args) {
  char cmd[256];
  sprintf(cmd, fmt, args...);
  return ::system(cmd);
}

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
void lock_question_file();
void unlock_question_file();

} // namespace Global

#endif
