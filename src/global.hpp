#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>
#include <set>
#include <vector>
#include <sstream>

#include "json.hpp"

void lock_settings();
void unlock_settings();
const JSON& settings_ref();

namespace Global {

extern std::map<int,JSON> attempts;

void install(const std::string&);
void start();
void stop();
void reload();
void rerun_attempt(int);

void lock_attempts();
void unlock_attempts();
void lock_question_file();
void unlock_question_file();

bool alive();
void shutdown();
void load_settings();

template <typename... Args>
JSON settings(Args... args) {
  lock_settings();
  JSON ans(std::move(settings_ref()(args...)));
  unlock_settings();
  return ans;
}

} // namespace Global

#endif
