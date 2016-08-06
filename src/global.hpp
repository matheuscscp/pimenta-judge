#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>
#include <set>
#include <vector>
#include <sstream>

#include "json.hpp"

enum {AC = 0, CE, RTE, TLE, WA, PE};
int verdict_toi(const std::string&);
std::string verdict_tos(int);
std::string verdict_tolongs(int);
std::string balloon_img(char);

struct Attempt {
  int id;
  char problem;
  char verdict;
  int when;
  std::string runtime;
  std::string username;
  std::string ip;
  std::string teamname;
  std::string status;
  bool read(FILE*);
  void write(FILE*) const;
  bool operator<(const Attempt&) const;
  std::string toHTMLtr(bool, bool) const;
  static std::string getHTMLtrheader();
};

template <typename NewType, typename T>
NewType to(const T& x) {
  std::stringstream ss; ss << x;
  NewType ret; ss >> ret;
  return ret;
}

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

void lock_settings();
void unlock_settings();
JSON& settings_ref();

namespace Global {

void install(const std::string&);
void start();
void stop();
void reload_settings();

void lock_att_file();
void unlock_att_file();
void lock_nextid_file();
void unlock_nextid_file();
void lock_question_file();
void unlock_question_file();

bool alive();
void shutdown();
void load_settings();

time_t remaining_time();

template <typename... Args>
JSON settings(Args... args) {
  lock_settings();
  JSON ans(settings_ref()(args...));
  unlock_settings();
  return ans;
}

} // namespace Global

#endif
