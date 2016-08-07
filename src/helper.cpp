#include "helper.hpp"

using namespace std;

int verdict_toi(const string& verd) {
  if (verd ==  "AC") return AC;
  if (verd ==  "CE") return CE;
  if (verd == "RTE") return RTE;
  if (verd == "TLE") return TLE;
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
    case  WA: return "WA";
    case  PE: return "PE";
  }
  return "";
}
