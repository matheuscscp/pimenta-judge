#ifndef GLOBAL_H
#define GLOBAL_H

#include <vector>
#include <string>
#include <sstream>

#include <netinet/in.h>

enum {AC = 0, CE, RTE, TLE, WA, PE};
int verdict_toi(const std::string&);
std::string verdict_tos(int);

struct Settings {
  time_t begin, end, freeze, noverdict;
  std::vector<int> problems;
  Settings();
};

struct Attempt {
  int id;
  char problem;
  char verdict;
  int when;
  char team[128];
  bool read(FILE*);
  void write(FILE*) const;
};

struct rejudgemsg {
  long mtype;
  int id;
  char verdict;
  rejudgemsg(int, char);
  size_t size() const;
};

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
std::string stringf(const char* fmt, Args... args) {
  char aux;
  int len = snprintf(&aux, 1, fmt, args...);
  char* buf = new char[len+1];
  sprintf(buf, fmt, args...);
  std::string ret = buf;
  delete[] buf;
  return ret;
}

template <typename... Args>
int system(const char* fmt, Args... args) {
  return ::system(stringf(fmt, args...).c_str());
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
