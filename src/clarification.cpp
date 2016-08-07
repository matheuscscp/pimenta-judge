#include <cstring>

#include <unistd.h>
#include <sys/stat.h>

#include "clarification.hpp"

#include "helper.hpp"
#include "global.hpp"

using namespace std;

namespace Clarification {

JSON query(const string& team) {
  struct clarification {
    JSON obj;
    bool read(FILE* fp, const string& team) {
      obj = JSON();
      string tmp;
      for (char c; fscanf(fp, "%c", &c) == 1 && c != ' '; tmp += c);
      if (tmp == "") return false;
      char problem;
      if (fscanf(fp, "%c", &problem) != 1) return false;
      obj["problem"] = to<string>(problem);
      fgetc(fp);
      fgetc(fp);
      string str;
      for (char c = fgetc(fp); c != '"' && c != EOF; str += c, c = fgetc(fp));
      obj["question"] = str;
      fgetc(fp);
      fgetc(fp);
      str = "";
      for (char c = fgetc(fp); c != '"' && c != EOF; str += c, c = fgetc(fp));
      obj["answer"] = str;
      fgetc(fp);
      if (tmp != "global" && tmp != team) obj = "false";
      return true;
    }
  };
  
  JSON ans(vector<JSON>({}));
  FILE* fp = fopen("clarifications.txt", "r");
  if (!fp) return ans;
  for (clarification c; c.read(fp, team);) if (c.obj) ans.push_back(c.obj);
  fclose(fp);
  return ans;
}

string question(const string& team, const string& problem, const string& text) {
  JSON contest(move(Global::settings("contest")));
  time_t begin = contest("begin");
  time_t end = contest("end");
  auto nproblems = contest("problems").size();
  
  // check time
  time_t now = time(nullptr);
  if (now < begin || end <= now) {
    return "The contest is not running.";
  }
  
  // check problem
  if (
    problem.size() == 0 || problem.size() > 1 ||
    problem[0] < 'A' || problem[0] > char('A' + nproblems - 1)
  ) {
    return "Choose a problem!";
  }
  
  // check text
  if (text.size() == 0) return "Write something!";
  
  // save
  mkdir("questions", 0777);
  char fn[17];
  strcpy(fn, "questions/XXXXXX");
  Global::lock_question_file();
  int fd = mkstemp(fn);
  dprintf(fd, "Team: %s\n", team.c_str());
  dprintf(fd, "Problem: %c\n", problem[0]);
  dprintf(fd, "Question: %s\n", text.c_str());
  close(fd);
  Global::unlock_question_file();
}

} // namespace Clarification
