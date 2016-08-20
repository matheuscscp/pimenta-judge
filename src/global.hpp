#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>

namespace Global {

void install(const std::string& path);
void start();
void stop();
void rerun_attempt(int id);

void shutdown();

} // namespace Global

#endif
