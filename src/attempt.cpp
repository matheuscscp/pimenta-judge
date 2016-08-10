#include "attempt.hpp"

#include "helper.hpp"
#include "global.hpp"

using namespace std;

Attempt::Attempt() : judged(false) {
  
}

Attempt::Attempt(JSON& json) {
  id        = json("id");
  problem   = json("problem").str();
  language  = json("language").str();
  verdict   = verdict_toi(json("verdict"));
  judged    = json("judged");
  when      = json("when");
  time      = json("time").str();
  memory    = json("memory").str();
  username  = json("username").str();
  ip        = json("ip").str();
}

JSON Attempt::json() const {
  return JSON(move(map<string,JSON>{
    {"id"       , id},
    {"problem"  , problem},
    {"language" , language},
    {"verdict"  , verdict_tos(verdict)},
    {"judged"   , judged ? "true" : "false"},
    {"when"     , when},
    {"time"     , time},
    {"memory"   , memory},
    {"username" , username},
    {"ip"       , ip}
  }));
}

bool Attempt::operator<(const Attempt& o) const {
  if (when != o.when) return when < o.when;
  return id < o.id;
}

void Attempt::store() const {
  Global::lock_attempts();
  Global::attempts[id] = move(json());
  Global::unlock_attempts();
}

void Attempt::commit(Attempt* att) {
  att->store();
  delete att;
}
