#include "attempt.hpp"

#include "global.hpp"

using namespace std;

Attempt::Attempt() : judged(false) {
  
}

Attempt::Attempt(int id, JSON& json) : id(id) {
  problem  = json("problem").str();
  verdict  = verdict_toi(json("verdict"));
  judged   = json("judged");
  when     = json("when");
  runtime  = json("runtime").str();
  username = json("username").str();
  ip       = json("ip").str();
}

JSON Attempt::json() const {
  return JSON(move(map<string,JSON>{
    {"problem" , problem},
    {"verdict" , verdict_tos(verdict)},
    {"judged"  , judged ? "true" : "false"},
    {"when"    , when},
    {"runtime" , runtime},
    {"username", username},
    {"ip"      , ip}
  }));
}

bool Attempt::operator<(const Attempt& o) const {
  if (when != o.when) return when < o.when;
  return id < o.id;
}
