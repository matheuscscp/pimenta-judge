#ifndef HELPER_H
#define HELPER_H

#include <string>

enum {AC = 0, CE, TLE, MLE, RTE, WA, PE};
int verdict_toi(const std::string&);
std::string verdict_tos(int);

template <typename NewType, typename T>
NewType to(const T& x) {
  std::stringstream ss; ss << x;
  NewType ret; ss >> ret;
  return ret;
}

template <typename T1, typename T2>
bool read(const T1& s, T2& t) {
  std::stringstream ss; ss << s;
  return ss >> t;
}

template <typename... Args>
std::string stringf(const char* fmt, Args... args) {
  char aux;
  std::string ret(snprintf(&aux,1,fmt,args...),' ');
  sprintf((char*)ret.c_str(),fmt,args...);
  return ret;
}

template <typename... Args>
int system(const char* fmt, Args... args) {
  return ::system(stringf(fmt, args...).c_str());
}

#endif
