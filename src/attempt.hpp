#ifndef ATTEMPT_H
#define ATTEMPT_H

#include "json.hpp"

struct Attempt {
  int id;
  std::string problem;
  std::string language;
  char verdict;
  bool judged;
  int when;
  std::string runtime;
  std::string username;
  std::string ip;
  Attempt();
  Attempt(int,JSON&);
  JSON json() const;
  bool operator<(const Attempt&) const;
};

#endif
