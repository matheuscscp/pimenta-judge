#include <unistd.h>

#include "helper.hpp"

using namespace std;

int verdict_toi(const string& verd) {
  if (verd ==  "AC") return AC;
  if (verd ==  "CE") return CE;
  if (verd == "RTE") return RTE;
  if (verd == "TLE") return TLE;
  if (verd == "MLE") return MLE;
  if (verd ==  "WA") return WA;
  if (verd ==  "PE") return PE;
  return -1;
}

string verdict_tos(int verd) {
  switch (verd) {
    case  AC: return "AC";
    case  CE: return "CE";
    case RTE: return "RTE";
    case TLE: return "TLE";
    case MLE: return "MLE";
    case  WA: return "WA";
    case  PE: return "PE";
  }
  return "";
}

string getcwd() {
  char* tmp = get_current_dir_name();
  string ans = tmp;
  free(tmp);
  return ans;
}

void debug() {
  fclose(fopen("debug","w"));
}
