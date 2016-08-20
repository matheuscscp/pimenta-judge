#include "attempt.hpp"

using namespace std;

namespace Attempt {

string create(JSON&& att, const vector<uint8_t>& src) {
  return "ok";
  /*
  // save file
  string path = "attempts/"+tostr(att.id);
  system("mkdir -p %s/output",path.c_str());
  FILE* fp = fopen((path+"/"+fn).c_str(), "wb");
  fwrite(&file[0], file.size(), 1, fp);
  fclose(fp);
  */
}

} // namespace Attempt
