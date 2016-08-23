#include "attempt.hpp"

#include "helper.hpp"
#include "problem.hpp"
#include "language.hpp"
#include "contest.hpp"
#include "database.hpp"
#include "judge.hpp"

using namespace std;

namespace Attempt {

string create(JSON&& att, const vector<uint8_t>& src) {
  JSON problem = Problem::get_short(att["problem"]);
  if (!problem) return "Problem "+att["problem"].str()+" do not exists!";
  JSON setts = Language::settings(att);
  if (!setts) return "Language "+att["language"].str()+" do not exists!";
  if (!Contest::allow_create_attempt(att,problem)) {
    return "The contest has not started yet!";
  }
  DB(attempts);
  int id = attempts.create(att);
  string fn = "attempts/"+tostr(id)+"/";
  system("mkdir -p %soutput",fn.c_str());
  fn += att["problem"].str();
  fn += att["language"].str();
  FILE* fp = fopen(fn.c_str(), "wb");
  fwrite(&src[0],src.size(),1,fp);
  fclose(fp);
  Judge::push(id);
  return "Attempt "+tostr(id)+" received.";
}

} // namespace Attempt
