#ifndef HELPER_H
#define HELPER_H

#include <string>

enum {AC = 0, CE, RTE, TLE, WA, PE};
int verdict_toi(const std::string&);
std::string verdict_tos(int);

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

#endif
