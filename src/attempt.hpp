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
  std::string time;
  std::string memory;
  std::string username;
  std::string ip;
  Attempt();
  Attempt(JSON&);
  JSON json() const;
  bool operator<(const Attempt&) const;
  void store() const;
  static void commit(Attempt*);
};

#endif
